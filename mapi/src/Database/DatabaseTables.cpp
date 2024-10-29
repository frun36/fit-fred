#include <sstream>
#include <algorithm>
#include "Database/DatabaseTables.h"

///
///     ParameteresTable
///

namespace db_tables
{

namespace equation
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
} // namespace equation

namespace boolean
{
bool parse(MultiBase* field)
{
    return (field->getString() == True);
}
const std::string True{ "Y" };
const std::string False{ "N" };
} // namespace boolean

namespace hex
{
uint32_t parse(MultiBase* field)
{
    std::stringstream ss;
    ss << field->getString();
    std::string hex(field->getString());
    uint32_t word = 0;
    ss >> std::hex >> word;
    return word;
}
} // namespace hex

// BoardTypes class
std::string BoardTypes::TypeTCM{ "TCM" };
std::string BoardTypes::TypePM{ "PM" };

// Parameters class
std::string Parameters::TableName{ "BOARD_PARAMETERS" };
Column Parameters::BoardType{ 0, "BOARD_TYPE" };
Column Parameters::Name{ 1, "PARAM_NAME" };
Column Parameters::BaseAddress{ 2, "BASE_ADDRESS" };
Column Parameters::StartBit{ 3, "START_BIT" };
Column Parameters::EndBit{ 4, "END_BIT" };
Column Parameters::RegBlockSize{ 5, "REG_BLOCK_SIZE" };
Column Parameters::MinValue{ 6, "MIN_VALUE" };
Column Parameters::MaxValue{ 7, "MAX_VALUE" };
Column Parameters::IsSigned{ 8, "IS_SIGNED" };
Column Parameters::IsFifo{ 9, "IS_FIFO" };
Column Parameters::IsReadOnly{ 10, "IS_READ_ONLY" };
Column Parameters::EqElectronicToPhysic{ 11, "EQ_ELECTRONIC_TO_PHYSIC" };
Column Parameters::EqPhysicToElectronic{ 12, "EQ_PHYSIC_TO_ELECTRONIC" };
Column Parameters::RefreshType{ 13, "REFRESH_TYPE" };
std::string Parameters::RefreshSYNC{ "SYNC" };
std::string Parameters::RefreshCNT{ "CNT" };

// ConnectedDevices class
std::string ConnectedDevices::TableName{ "CONNECTED_DEVICES" };
Column ConnectedDevices::BoardName{ 0, "BOARD_NAME" };
Column ConnectedDevices::BoardType{ 1, "BOARD_TYPE" };
std::string ConnectedDevices::TypePM{ "PM" };
std::string ConnectedDevices::TypeTCM{ "TCM" };

// Environment class
std::string Environment::TableName{ "FEE_SETTINGS" };
Column Environment::Name{ 0, "NAME" };
Column Environment::Equation{ 1, "EQUATION" };

// Configurations class
std::string Configurations::TableName{ "CONFIGURATIONS" };
Column Configurations::ConfigurationName{ 0, "CONFIGURATION_NAME" };

// ConfigurationParameters class
std::string ConfigurationParameters::TableName{ "CONFIGURATION_PARAMETERS" };
Column ConfigurationParameters::ConfigurationName{ 0, "CONFIGURATION_NAME" };
Column ConfigurationParameters::BoardName{ 1, "BOARD_NAME" };
Column ConfigurationParameters::BoardType{ 2, "BOARD_TYPE" };
Column ConfigurationParameters::ParameterName{ 3, "PARAMETER_NAME" };
Column ConfigurationParameters::Value{ 4, "PARAMETER_VALUE" };

} // namespace db_tables