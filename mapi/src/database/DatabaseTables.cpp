#include <sstream>
#include <algorithm>
#include "database/DatabaseTables.h"

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
const std::string BoardTypes::TypeTCM{ "TCM" };
const std::string BoardTypes::TypePM{ "PM" };

// Parameters class
const std::string Parameters::TableName{ "BOARD_PARAMETERS" };
const Column Parameters::BoardType{ 0, "BOARD_TYPE" };
const Column Parameters::Name{ 1, "PARAM_NAME" };
const Column Parameters::BaseAddress{ 2, "BASE_ADDRESS" };
const Column Parameters::StartBit{ 3, "START_BIT" };
const Column Parameters::EndBit{ 4, "END_BIT" };
const Column Parameters::RegBlockSize{ 5, "REG_BLOCK_SIZE" };
const Column Parameters::MinValue{ 6, "MIN_VALUE" };
const Column Parameters::MaxValue{ 7, "MAX_VALUE" };
const Column Parameters::IsSigned{ 8, "IS_SIGNED" };
const Column Parameters::IsFifo{ 9, "IS_FIFO" };
const Column Parameters::IsReadOnly{ 10, "IS_READ_ONLY" };
const Column Parameters::IsAutoReset{ 11, "AUTO-RESET"};
const Column Parameters::EqElectronicToPhysic{ 12, "EQ_ELECTRONIC_TO_PHYSIC" };
const Column Parameters::EqPhysicToElectronic{ 13, "EQ_PHYSIC_TO_ELECTRONIC" };
const Column Parameters::RefreshType{ 14, "REFRESH_TYPE" };
const std::string Parameters::RefreshSYNC{ "SYNC" };
const std::string Parameters::RefreshCNT{ "CNT" };

// ConnectedDevices class
const std::string ConnectedDevices::TableName{ "CONNECTED_DEVICES" };
const Column ConnectedDevices::BoardName{ 0, "BOARD_NAME" };
const Column ConnectedDevices::BoardType{ 1, "BOARD_TYPE" };
const Column ConnectedDevices::IsConnected{ 2, "IS_CONNECTED" };
const std::string ConnectedDevices::TypePM{ "PM" };
const std::string ConnectedDevices::TypeTCM{ "TCM" };

// Environment class
const std::string Environment::TableName{ "FEE_SETTINGS" };
const Column Environment::Name{ 0, "NAME" };
const Column Environment::Equation{ 1, "EQUATION" };

// Configurations class
const std::string Configurations::TableName{ "CONFIGURATIONS" };
const Column Configurations::ConfigurationName{ 0, "CONFIGURATION_NAME" };

// ConfigurationParameters class
const std::string ConfigurationParameters::TableName{ "CONFIGURATION_PARAMETERS" };
const Column ConfigurationParameters::ConfigurationName{ 0, "CONFIGURATION_NAME" };
const Column ConfigurationParameters::BoardName{ 1, "BOARD_NAME" };
const Column ConfigurationParameters::BoardType{ 2, "BOARD_TYPE" };
const Column ConfigurationParameters::ParameterName{ 3, "PARAMETER_NAME" };
const Column ConfigurationParameters::ParameterValue{ 4, "PARAMETER_ELECTRONIC_VALUE" };

} // namespace db_tables