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
    static const std::string TypeTCM;
    static const std::string TypePM;
};

class Parameters
{
   public:
    static const std::string TableName;

    static const Column BoardType;
    static const Column Name;
    static const Column BaseAddress;
    static const Column StartBit;
    static const Column EndBit;
    static const Column RegBlockSize;
    static const Column MinValue;
    static const Column MaxValue;
    static const Column IsSigned;
    static const Column IsFifo;
    static const Column IsReadOnly;
    static const Column EqElectronicToPhysic;
    static const Column EqPhysicToElectronic;
    static const Column RefreshType;

    static const std::string RefreshSYNC;
    static const std::string RefreshCNT;
};

class ConnectedDevices
{
   public:
    static const std::string TableName;

    static const Column BoardName;
    static const Column BoardType;

    static const std::string TypePM;
    static const std::string TypeTCM;
};

class Environment
{
   public:
    static const std::string TableName;

    static const Column Name;
    static const Column Equation;
};

class Configurations
{
   public:
    static const std::string TableName;

    static const Column ConfigurationName;
};

class ConfigurationParameters
{
   public:
    static const std::string TableName;

    static const Column ConfigurationName;
    static const Column BoardName;
    static const Column BoardType;
    static const Column ParameterName;
    static const Column Value;
};

} // namespace db_tables
