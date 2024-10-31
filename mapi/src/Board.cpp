#include <stdexcept>
#include "Board.h"
#include "utils.h"

#ifdef FIT_UNIT_TEST

#include "../../../test/mocks/include/utility.h"

#else

#include "Parser/utility.h"

#endif

Board::Board(std::string name, uint32_t address, std::shared_ptr<Board> main, std::shared_ptr<EnvironmentVariables> settings) : m_name(name), m_address(address), m_mainBoard(main), m_environmentalVariables(settings)
{
    if (name.find("TCM") != std::string::npos) {
        m_identity.type = Type::TCM;
        m_identity.side = std::nullopt;
        m_identity.number = 0;
    } else {
        m_identity.type = Type::PM;
        m_identity.side = (name.find("A") != std::string::npos) ? Side::A : Side::C;
        m_identity.number = name.back() - '0';
    }
}

Board::ParameterInfo::ParameterInfo(const Board::ParameterInfo& base, uint32_t boardAddress) : name(base.name),
                                                                                               baseAddress(base.baseAddress + boardAddress),
                                                                                               startBit(base.startBit),
                                                                                               bitLength(base.bitLength),
                                                                                               regBlockSize(base.regBlockSize),
                                                                                               valueEncoding(base.valueEncoding),
                                                                                               minValue(base.minValue),
                                                                                               maxValue(base.maxValue),
                                                                                               electronicToPhysic(base.electronicToPhysic),
                                                                                               physicToElectronic(base.physicToElectronic),
                                                                                               isFifo(base.isFifo),
                                                                                               isReadonly(base.isReadonly),
                                                                                               refreshType(base.refreshType),
                                                                                               m_value(std::nullopt)
{
}

Board::ParameterInfo::ParameterInfo(
    std::string name_,
    uint32_t baseAddress_,
    uint32_t startBit_,
    uint32_t bitLength_,
    uint32_t regBlockSize_,
    ValueEncoding valueEncoding_,
    int64_t minValue_,
    int64_t maxValue_,
    Equation electronicToPhysic_,
    Equation physicToRaw_,
    bool isFifo_,
    bool isReadonly_,
    RefreshType refreshType_) : name(name_),
                                baseAddress(baseAddress_),
                                startBit(startBit_),
                                bitLength(bitLength_),
                                regBlockSize(regBlockSize_),
                                valueEncoding(valueEncoding_),
                                minValue(minValue_),
                                maxValue(maxValue_),
                                electronicToPhysic(electronicToPhysic_),
                                physicToElectronic(physicToRaw_),
                                isFifo(isFifo_),
                                isReadonly(isReadonly_),
                                refreshType(refreshType_),
                                m_value(std::nullopt) {}

bool Board::emplace(const ParameterInfo& info)
{
    return m_parameters.emplace(info.name, ParameterInfo(info, m_address)).second;
}

bool Board::emplace(ParameterInfo&& info)
{
    return m_parameters.emplace(info.name, ParameterInfo(info, m_address)).second;
}

Board::ParameterInfo& Board::operator[](const std::string& param)
{
    return at(param);
}

Board::ParameterInfo& Board::operator[](std::string_view param)
{
    return at(param);
}

// Board::ParameterInfo& Board::at(const std::string& param)
// {
//     try{
//         Board::ParameterInfo& ref = m_parameters.at(param);
//         return ref;
//     }
//     catch (const std::out_of_range&) {
//         throw std::out_of_range("Parameter " + param + " not found on the board.");
//     }
// }

Board::ParameterInfo& Board::at(std::string_view param)
{
    try {
        Board::ParameterInfo& ref = m_parameters.at(param.data());
        return ref;
    } catch (const std::out_of_range&) {
        throw std::out_of_range("Parameter " + std::string(param) + " not found on the board.");
    }
}

bool Board::doesExist(const std::string& param)
{
    return m_parameters.find(param) != m_parameters.end();
}

double Board::getEnvironment(const std::string& variableName)
{
    return m_environmentalVariables->getVariable(variableName);
}

void Board::setEnvironment(const std::string& variableName, double value)
{
    m_environmentalVariables->setVariable(variableName, value);
}

void Board::updateEnvironment(const std::string& variableName)
{
    m_environmentalVariables->updateVariable(variableName);
}

double Board::calculatePhysical(const std::string& param, uint32_t raw) const
{
    if (m_parameters.find(param) == m_parameters.end()) {
        throw std::runtime_error(param + " does not exist!");
    }

    const ParameterInfo& info = m_parameters.at(param);
    int64_t decoded = 0;

    if (info.valueEncoding == ParameterInfo::ValueEncoding::Unsigned) {
        decoded = static_cast<int64_t>(getBitField(raw, info.startBit, info.bitLength));
    } else {
        decoded = static_cast<int64_t>(twosComplementDecode<int32_t>(getBitField(raw, info.startBit, info.bitLength), info.bitLength));
    }

    if (info.electronicToPhysic.equation == "") {
        return decoded;
    }

    std::vector<double> values;
    for (const auto& var : info.electronicToPhysic.variables) {
        if (var == info.name) {
            values.emplace_back(decoded);
        } else if (m_parameters.find(var) != m_parameters.end()) {
            values.emplace_back(m_parameters.at(var).getStoredValue());
        } else if (m_mainBoard != nullptr && m_mainBoard->doesExist(var)) {
            values.emplace_back(m_mainBoard->at(var).getStoredValue());
        } else if (m_environmentalVariables != nullptr && m_environmentalVariables->doesExist(var)) {
            values.emplace_back(m_environmentalVariables->getVariable(var));
        } else {
            throw std::runtime_error("Parameter " + var + " does not exist!");
        }
    }
    if (values.size() != info.electronicToPhysic.variables.size()) {
        throw std::runtime_error("Parameter " + param + ": parsing equation failed!");
    }
    std::string equation = info.electronicToPhysic.equation;
    std::vector<std::string> variables = info.electronicToPhysic.variables;
    return Utility::calculateEquation(equation, variables, values);
}

int64_t Board::calculateElectronic(const std::string& param, double physical) const
{
    if (m_parameters.find(param) == m_parameters.end()) {
        throw std::runtime_error(param + " does not exist!");
    }

    const ParameterInfo& info = m_parameters.at(param);

    if (info.physicToElectronic.equation == "") {
        return static_cast<int64_t>(physical);
    }

    std::vector<double> values;
    for (const auto& var : info.electronicToPhysic.variables) {
        if (var == info.name) {
            values.emplace_back(physical);
            continue;
        } else if (m_parameters.find(var) != m_parameters.end()) {
            values.emplace_back(m_parameters.at(var).getStoredValue());
        } else if (m_mainBoard != nullptr && m_mainBoard->doesExist(var)) {
            values.emplace_back(m_mainBoard->at(var).getStoredValue());
        } else if (m_environmentalVariables != nullptr && m_environmentalVariables->doesExist(var)) {
            values.emplace_back(m_environmentalVariables->getVariable(var));
        } else {
            throw std::runtime_error("Parameter " + var + " does not exist!");
        }
    }

    std::string equation = info.physicToElectronic.equation;
    std::vector<std::string> variables = info.physicToElectronic.variables;

    return static_cast<int64_t>(Utility::calculateEquation(equation, variables, values));
}

uint32_t Board::convertElectronicToRaw(const std::string& param, int64_t physcial) const
{
    if (m_parameters.find(param) == m_parameters.end()) {
        throw std::runtime_error(param + " does not exist!");
    }

    const ParameterInfo& info = m_parameters.at(param);

    if (info.valueEncoding != ParameterInfo::ValueEncoding::Unsigned) {
        return twosComplementEncode(physcial, info.bitLength) << info.startBit;
    }

    return static_cast<uint32_t>(physcial) << info.startBit;
}