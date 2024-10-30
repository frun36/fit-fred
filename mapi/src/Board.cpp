#include <stdexcept>
#include "Board.h"
#include "utils.h"

#ifdef FIT_UNIT_TEST

#include "../../../test/mocks/include/utility.h"

#else

#include "Parser/utility.h"

#endif

Board::Board(std::string name, uint32_t address, std::shared_ptr<Board> main, std::shared_ptr<EnvironmentVariables> settings) : m_name(name), m_address(address), m_mainBoard(main), m_settings(settings)
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
    uint8_t startBit_,
    uint8_t bitLength_,
    uint32_t regBlockSize_,
    ValueEncoding valueEncoding_,
    double minValue_,
    double maxValue_,
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
    return m_settings->getVariable(variableName);
}

void Board::setEnvironment(const std::string& variableName, double value)
{
    m_settings->setVariable(variableName, value);
}

void Board::updateEnvironment(const std::string& variableName)
{
    m_settings->updateVariable(variableName);
}

double Board::calculatePhysical(const std::string& param, uint32_t raw)
{
    if (m_parameters.find(param) == m_parameters.end()) {
        throw std::runtime_error(param + " does not exist!");
    }

    ParameterInfo& info = m_parameters.at(param);
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
        } else if (m_settings != nullptr && m_settings->doesExist(var)) {
            values.emplace_back(m_settings->getVariable(var));
        } else {
            throw std::runtime_error("Parameter " + var + " does not exist!");
        }
    }
    if (values.size() != info.electronicToPhysic.variables.size()) {
        throw std::runtime_error("Parameter " + param + ": parsing equation failed!");
    }
    std::string equation = info.electronicToPhysic.equation;
    return Utility::calculateEquation(equation, info.electronicToPhysic.variables, values);
}

uint32_t Board::calculateRaw(const std::string& param, double physical)
{
    if (m_parameters.find(param) == m_parameters.end()) {
        throw std::runtime_error(param + " does not exist!");
    }

    ParameterInfo& info = m_parameters.at(param);

    if (info.physicToElectronic.equation == "") {
        if (info.valueEncoding != ParameterInfo::ValueEncoding::Unsigned) {
            return twosComplementEncode(static_cast<int32_t>(physical), info.bitLength) << info.startBit;
        }
        return static_cast<uint32_t>(physical) << info.startBit;
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
        } else if (m_settings != nullptr && m_settings->doesExist(var)) {
            values.emplace_back(m_settings->getVariable(var));
        } else {
            throw std::runtime_error("Parameter " + var + " does not exist!");
        }
    }

    int32_t calculated = static_cast<int32_t>(Utility::calculateEquation(info.physicToElectronic.equation, info.physicToElectronic.variables, values));

    if (info.valueEncoding != ParameterInfo::ValueEncoding::Unsigned) {
        return twosComplementEncode(calculated, info.bitLength) << info.startBit;
    }

    return static_cast<uint32_t>(calculated) << info.startBit;
}
