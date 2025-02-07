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

template<typename DataType, DataType(*DataParser)(MultiBase*)>
struct Column {
    Column(uint8_t _idx, std::string _name, std::optional<DataType> _default = std::nullopt) : 
        idx(_idx), name(_name), defaultValue(_default) {}

    const uint8_t idx;
    const std::string name;
    std::optional<DataType> defaultValue;

    DataType parse(MultiBase* data) const
    {
        if(data == nullptr && defaultValue.has_value()){
            return *defaultValue;
        } else if(data == nullptr){
            throw std::runtime_error(name + ": must not be NULL!");
        }
        return DataParser(data);
    }

    std::optional<DataType> parseNullable(MultiBase* data) const
    {
        if(data == nullptr){
            return std::nullopt;
        }
        return DataParser(data);
    }
};

namespace parsers
{
        
bool booleanParser(MultiBase* data);
uint32_t hexParser(MultiBase* data);
int64_t integerParser(MultiBase* data);
uint32_t unsignedParser(MultiBase* data);
std::string stringParser(MultiBase* data);
Equation equationParser(MultiBase*);

}

typedef Column<bool, parsers::booleanParser> BooleanColumn;
typedef Column<uint32_t, parsers::hexParser> HexColumn;
typedef Column<int64_t, parsers::integerParser> IntegerColumn;
typedef Column<uint32_t, parsers::unsignedParser> UnsignedColumn;
typedef Column<std::string, parsers::stringParser> StringColumn;
typedef Column<Equation, parsers::equationParser> EquationColumn;

}