#pragma once

#include <string_view>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include<vector>
#include "Board.h"
#include "Database/databaseinterface.h"
#include "utils.h"
#include"Columns.h"

namespace db_fit
{

namespace tabels
{

std::string fullColumnName(const std::string& tabelName, const std::string& columnName);

class BoardTypes
{
   public:
    static const StringColumn board_type;

    static const std::string TypeTCM;
    static const std::string TypePM;
};

class Parameters
{
   public:
    static const std::string TableName;

    static const StringColumn BoardType;
    static const StringColumn Name;
    static const HexColumn BaseAddress;
    static const UnsignedColumn StartBit;
    static const UnsignedColumn EndBit;
    static const UnsignedColumn RegBlockSize;
    static const IntegerColumn MinValue;
    static const IntegerColumn MaxValue;
    static const BooleanColumn IsSigned;
    static const BooleanColumn IsFifo;
    static const BooleanColumn IsReadOnly;
    static const EquationColumn EqElectronicToPhysic;
    static const EquationColumn EqPhysicToElectronic;
    static const StringColumn RefreshType;

    struct Row
    {
        Row(const std::vector<MultiBase*>& row);
        std::string boardType;
        std::string name;
        uint32_t baseAddress;
        uint32_t startBit;
        uint32_t endBit;
        uint32_t regBlockSize;
        std::optional<int64_t> minValue;
        std::optional<int64_t> maxValue;
        bool isSigned;
        bool isFifo;
        bool isReadOnly;
        Equation eqElectronicToPhysic;
        Equation eqPhysicToElectronic;
        std::optional<std::string> refreshType;
    };
    
    static const std::string RefreshSYNC;
    static const std::string RefreshCNT;
};

class ConnectedDevices
{
   public:
    static const std::string TableName;

    static const StringColumn BoardName;
    static const StringColumn BoardType;
    static const BooleanColumn IsConnected;

    struct Row
    {
        Row(const std::vector<MultiBase*>& row);
        std::string boardName;
        std::string boardType;
        bool isConnected;
    };
    

    static const std::string TypePM;
    static const std::string TypeTCM;
};

class Environment
{
   public:
    static const std::string TableName;

    static const StringColumn Name;
    static const EquationColumn VariableEquation;

    struct Row
    {
        Row(const std::vector<MultiBase*>& row);
        std::string name;
        Equation equation;
    };
};

class Configurations
{
   public:
    static const std::string TableName;

    static const StringColumn ConfigurationName;
    static const StringColumn Author;
    static const StringColumn Date;
    static const StringColumn Comment;

    struct Row
    {
        Row(const std::vector<MultiBase*>& row);
        std::string configuratioName;
        std::string author;
        std::string date;
        std::string comment;
    };
};

class ConfigurationParameters
{
   public:
    static const std::string TableName;

    static const StringColumn ConfigurationName;
    static const StringColumn BoardName;
    static const StringColumn BoardType;
    static const StringColumn ParameterName;
    static const IntegerColumn ParameterValue;

    struct Row
    {
        Row(const std::vector<MultiBase*>& row);
        std::string configuratioName;
        std::string boardName;
        std::string boardType;
        std::string parameterName;
        int64_t parameterValue;
    };
};

class PmChannelHistograms
{
public:
    static const std::string TableName;
    
    static const UnsignedColumn Id;
    static const StringColumn Name;
};

class PmChannelHistogramStructure
{
public:
    static const std::string TableName;

    static const UnsignedColumn Id;
    static const UnsignedColumn PmHistogramId;
    static const HexColumn BaseAddress;
    static const UnsignedColumn RegBlockSize;
    static const IntegerColumn StartBin;
    static const UnsignedColumn BinsPerRegister;
    static const StringColumn BinDirection;
};

}

namespace views
{
class Histogram
{
public:
    static const StringColumn HistogramName;
    static const HexColumn BaseAddress;
    static const IntegerColumn StartBin;
    static const UnsignedColumn RegBlockSize;
    static const StringColumn Direction;

    struct Row
    {
        Row(const std::vector<MultiBase*>& row);
        std::string histogramName;
        uint32_t baseAddress;
        int32_t startBin;
        uint32_t regBlockSize;
        std::string direction;
    };
};
}


} // namespace db_fit
