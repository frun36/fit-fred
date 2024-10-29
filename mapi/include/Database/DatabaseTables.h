#pragma once

#include <string_view>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include "Board.h"
#include "Database/databaseinterface.h"
#include "utils.h"

namespace db_tables
{

struct Column {
    Column(uint8_t _idx, std::string _name) : idx(_idx), name(_name) {}

    uint8_t idx;
    std::string name;
};

namespace boolean
{
extern const std::string True;
extern const std::string False;
bool parse(MultiBase* field);
} // namespace boolean

namespace hex
{
uint32_t parse(MultiBase* field);
}

namespace equation
{
Equation parseEquation(std::string equation);
}

class BoardTypes
{
   public:
    static std::string TypeTCM;
    static std::string TypePM;
};

class Parameters
{
   public:
    static std::string TableName;

    static Column BoardType;
    static Column Name;
    static Column BaseAddress;
    static Column StartBit;
    static Column EndBit;
    static Column RegBlockSize;
    static Column MinValue;
    static Column MaxValue;
    static Column IsSigned;
    static Column IsFifo;
    static Column IsReadOnly;
    static Column EqElectronicToPhysic;
    static Column EqPhysicToElectronic;
    static Column RefreshType;

    static std::string RefreshSYNC;
    static std::string RefreshCNT;
};

class ConnectedDevices
{
   public:
    static std::string TableName;

    static Column BoardName;
    static Column BoardType;

    static std::string TypePM;
    static std::string TypeTCM;
};

class Environment
{
   public:
    static std::string TableName;

    static Column Name;
    static Column Equation;
};

class Configurations
{
   public:
    static std::string TableName;

    static Column ConfigurationName;
};

class ConfigurationParameters
{
   public:
    static std::string TableName;

    static Column ConfigurationName;
    static Column BoardName;
    static Column BoardType;
    static Column ParameterName;
    static Column Value;
};

} // namespace db_tables
