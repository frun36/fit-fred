#include <sstream>
#include <algorithm>
#include<vector>
#include "database/DatabaseTables.h"

///
///     ParameteresTable
///

namespace db_tables
{


// namespace hex

// BoardTypes class
const std::string BoardTypes::TypeTCM{ "TCM" };
const std::string BoardTypes::TypePM{ "PM" };

// Parameters class
const std::string Parameters::TableName{ "BOARD_PARAMETERS" };
const StringColumn Parameters::BoardType{ 0, "BOARD_TYPE" };
const StringColumn Parameters::Name{ 1, "PARAM_NAME" };
const HexColumn Parameters::BaseAddress{ 2, "BASE_ADDRESS" };
const UnsignedColumn Parameters::StartBit{ 3, "START_BIT" };
const UnsignedColumn Parameters::EndBit{ 4, "END_BIT" };
const UnsignedColumn Parameters::RegBlockSize{ 5, "REG_BLOCK_SIZE" };
const IntegerColumn Parameters::MinValue{ 6, "MIN_VALUE" };
const IntegerColumn Parameters::MaxValue{ 7, "MAX_VALUE" };
const BooleanColumn Parameters::IsSigned{ 8, "IS_SIGNED" };
const BooleanColumn Parameters::IsFifo{ 9, "IS_FIFO" };
const BooleanColumn Parameters::IsReadOnly{ 10, "IS_READ_ONLY" };
const EquationColumn Parameters::EqElectronicToPhysic{ 11, "EQ_ELECTRONIC_TO_PHYSIC", Equation::Empty() };
const EquationColumn Parameters::EqPhysicToElectronic{ 12, "EQ_PHYSIC_TO_ELECTRONIC", Equation::Empty() };
const StringColumn Parameters::RefreshType{ 13, "REFRESH_TYPE", "NOT" };
const std::string Parameters::RefreshSYNC{ "SYNC" };
const std::string Parameters::RefreshCNT{ "CNT" };

Parameters::Row::Row(const std::vector<MultiBase*>& row)
{
    boardType = Parameters::BoardType.parse(row[Parameters::BoardType.idx]);
    name = Parameters::Name.parse(row[Parameters::Name.idx]);
    baseAddress = Parameters::BaseAddress.parse(row[Parameters::BaseAddress.idx]);
    startBit = Parameters::StartBit.parse(row[Parameters::StartBit.idx]);
    endBit = Parameters::EndBit.parse(row[Parameters::EndBit.idx]);
    regBlockSize = Parameters::RegBlockSize.parse(row[Parameters::RegBlockSize.idx]);
    minValue = Parameters::MinValue.parse(row[Parameters::MinValue.idx]);
    maxValue = Parameters::MaxValue.parse(row[Parameters::MaxValue.idx]);
    isSigned = Parameters::IsSigned.parse(row[Parameters::IsSigned.idx]);
    isReadOnly = Parameters::IsSigned.parse(row[Parameters::IsReadOnly.idx]);
    isFifo = Parameters::IsFifo.parse(row[Parameters::IsReadOnly.idx]);
    eqElectronicToPhysic = Parameters::EqElectronicToPhysic.parse(row[Parameters::EqElectronicToPhysic.idx]);
    eqPhysicToElectronic = Parameters::EqPhysicToElectronic.parse(row[Parameters::EqPhysicToElectronic.idx]);
    refreshType = Parameters::RefreshType.parse(row[Parameters::RefreshType.idx]);
}


// ConnectedDevices class
const std::string ConnectedDevices::TableName{ "CONNECTED_DEVICES" };
const StringColumn ConnectedDevices::BoardName{ 0, "BOARD_NAME" };
const StringColumn ConnectedDevices::BoardType{ 1, "BOARD_TYPE" };
const BooleanColumn ConnectedDevices::IsConnected{ 2, "IS_CONNECTED" };
const std::string ConnectedDevices::TypePM{ "PM" };
const std::string ConnectedDevices::TypeTCM{ "TCM" };

ConnectedDevices::Row::Row(const std::vector<MultiBase*>& row)
{
    boardName = ConnectedDevices::BoardName.parse(row[ConnectedDevices::BoardName.idx]);
    boardType = ConnectedDevices::BoardType.parse(row[ConnectedDevices::BoardType.idx]);
    isConnected = ConnectedDevices::IsConnected.parse(row[ConnectedDevices::IsConnected.idx]);
}

// Environment class
const std::string Environment::TableName{ "FEE_SETTINGS" };
const StringColumn Environment::Name{ 0, "NAME" };
const EquationColumn Environment::VariableEquation{ 1, "EQUATION" };

Environment::Row::Row(const std::vector<MultiBase*>& row)
{
    name = Environment::Name.parse(row[Environment::Name.idx]);
    equation = Environment::VariableEquation.parse(row[Environment::VariableEquation.idx]);
}

// Configurations class
const std::string Configurations::TableName{ "CONFIGURATIONS" };
const StringColumn Configurations::ConfigurationName{ 0, "CONFIGURATION_NAME" };
const StringColumn Configurations::Author{ 1, "AUTHOR" };
const StringColumn Configurations::Date{ 2, "TIME_UPDATED" };
const StringColumn Configurations::Comment{ 3, "COMMENTS" };

Configurations::Row::Row(const std::vector<MultiBase*>& row)
{
    configuratioName = ConfigurationName.parse(row[ConfigurationName.idx]);
    author = Author.parse(row[Author.idx]);
    date = Date.parse(row[Date.idx]);
    comment = Comment.parse(row[Comment.idx]);
}

// ConfigurationParameters class
const std::string ConfigurationParameters::TableName{ "CONFIGURATION_PARAMETERS" };
const StringColumn ConfigurationParameters::ConfigurationName{ 0, "CONFIGURATION_NAME" };
const StringColumn ConfigurationParameters::BoardName{ 1, "BOARD_NAME" };
const StringColumn ConfigurationParameters::BoardType{ 2, "BOARD_TYPE" };
const StringColumn ConfigurationParameters::ParameterName{ 3, "PARAMETER_NAME" };
const IntegerColumn ConfigurationParameters::ParameterValue{ 4, "PARAMETER_ELECTRONIC_VALUE" };

ConfigurationParameters::Row::Row(const std::vector<MultiBase*>& row)
{
    configuratioName = ConfigurationName.parse(row[ConfigurationName.idx]);
    boardName = BoardName.parse(row[BoardName.idx]);
    boardType = BoardType.parse(row[BoardType.idx]);
    parameterName = ParameterName.parse(row[ParameterName.idx]);
    parameterValue = ParameterValue.parse(row[ParameterValue.idx]);
}

} // namespace db_tables