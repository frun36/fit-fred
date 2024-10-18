#include <sstream>
#include <algorithm>
#include "DatabaseUtils.h"

///
///     ParameteresTable
///

namespace db_utils
{

Equation parseEquation(std::string equation)
{
    Equation parsed;
    size_t left = 0;
    size_t right = 0;
    while ((left = equation.find('{', left)) != std::string::npos) {
        if ((right = equation.find('}', left)) == std::string::npos) {
            throw std::runtime_error("Invalid equation, missing }");
        }
        parsed.variables.emplace_back(equation.substr(left + 1, right - left - 1));
        left = right;
    }
    equation.erase(std::remove(equation.begin(), equation.end(), '{'), equation.end());
    equation.erase(std::remove(equation.begin(), equation.end(), '}'), equation.end());
    parsed.equation = std::move(equation);
    return parsed;
}

std::string selectQuery(std::string_view tableName, const std::vector<std::string_view>& columns, const std::vector<std::string_view>& whereConditions, bool distinct)
{
    std::stringstream ss;
    ss << "SELECT ";
    if (distinct) {
        ss << "DISTINCT ";
    }
    bool first = true;
    for (auto& column : columns) {
        if (!first) {
            ss << ',';
        }
        ss << column;
        first = false;
    }

    ss << " FROM " << tableName;

    if (!whereConditions.empty()) {
        first = true;
        ss << " WHERE ";
        for (auto& whereC : whereConditions) {
            if (!first) {
                ss << ',';
            }
            ss << whereC;
            first = false;
        }
    }
    return ss.str();
}

} // namespace db_utils

bool ParametersTable::parseBoolean(MultiBase* field)
{
    return (field->getString() == Parameter::ExprTRUE);
}

uint32_t ParametersTable::parseHex(MultiBase* field)
{
    std::stringstream ss;
    ss << field->getString();
    std::string hex(field->getString());
    uint32_t word = 0;
    ss >> std::hex >> word;
    return word;
}

Board::ParameterInfo ParametersTable::Parameter::buildParameter(std::vector<MultiBase*>& dbRow)
{
    Equation electronicToPhysic = (dbRow[Parameter::EqElectronicToPhysic] == NULL) ? Equation::Empty()
                                                                                   : db_utils::parseEquation(dbRow[Parameter::EqElectronicToPhysic]->getString());

    Equation physicToElectronic = (dbRow[Parameter::EqPhysicToElectronic] == NULL) ? Equation::Empty()
                                                                                   : db_utils::parseEquation(dbRow[Parameter::EqPhysicToElectronic]->getString());

    Board::ParameterInfo::RefreshType refreshType = Board::ParameterInfo::RefreshType::NOT;

    if (dbRow[Parameter::RefreshType] != NULL) {

        if (dbRow[Parameter::RefreshType]->getString() == RefreshCNT) {
            refreshType = Board::ParameterInfo::RefreshType::CNT;
        } else if (dbRow[Parameter::RefreshType]->getString() == RefreshSYNC) {
            refreshType = Board::ParameterInfo::RefreshType::SYNC;
        }
    }

    Board::ParameterInfo::ValueEncoding encoding = parseBoolean(dbRow[IsSigned]) ? Board::ParameterInfo::ValueEncoding::Signed : Board::ParameterInfo::ValueEncoding::Unsigned;

    uint32_t bitLength = dbRow[Parameter::EndBit]->getInt() - dbRow[Parameter::StartBit]->getInt() + 1;

    double max = 0;
    double min = 0;
    if (dbRow[Parameter::MinValue] == NULL) {
        min = static_cast<double>(
            (encoding == Board::ParameterInfo::ValueEncoding::Unsigned) ? 0 : twosComplementDecode<int32_t>((1u << (bitLength - 1)), bitLength));
    } else {
        min = dbRow[Parameter::MinValue]->getDouble();
    }

    if (dbRow[Parameter::MaxValue] == NULL) {
        max = static_cast<double>(
            (encoding == Board::ParameterInfo::ValueEncoding::Unsigned) ? ((1u << bitLength) - 1) : twosComplementDecode<int32_t>((1u << (bitLength - 1)) - 1, bitLength));
    } else {
        max = dbRow[Parameter::MaxValue]->getDouble();
    }

    return {
        dbRow[Parameter::Name]->getString(),
        parseHex(dbRow[Parameter::BaseAddress]),
        static_cast<uint8_t>(dbRow[Parameter::StartBit]->getInt()),
        static_cast<uint8_t>(bitLength),
        static_cast<uint8_t>(dbRow[Parameter::RegBlockSize]->getInt()),
        encoding,
        min,
        max,
        electronicToPhysic,
        physicToElectronic,
        parseBoolean(dbRow[Parameter::IsFifo]),
        parseBoolean(dbRow[Parameter::IsReadOnly]),
        refreshType
    };
}

std::string ParametersTable::selectBoardParameters(std::string_view boardType)
{
    return db_utils::selectQuery(ParametersTable::Name, { "*" }, { db_utils::where(ParametersTable::Parameter::BoardTypeName, "=", boardType) });
}

///
///     ConnectedDevicesTable
///

ConnectedDevicesTable::Device::Device(std::vector<MultiBase*>& dbRow)
{
    name = dbRow[Name]->getString();
    std::string type = dbRow[Type]->getString();

    if (type == TypePM)
        this->type = BoardType::PM;
    else if (type == TypeTCM)
        this->type = BoardType::TCM;
    else
        throw std::runtime_error("Invalid board type in Connected Devices table");

    if (this->type == BoardType::PM) {
        if (name.find("C") != std::string::npos)
            this->side = Side::C;
        else
            this->side = Side::A;

        this->index = std::stoi(name.substr(3, 1));
    }
}
