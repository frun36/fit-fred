#pragma once

#include <string_view>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include<tuple>
#include "Board.h"
#include "Database/databaseinterface.h"
#include "utils.h"

namespace db_utils
{

namespace parsers
{
    bool parseBoolean(MultiBase*);
    uint32_t parseHex(MultiBase*);
    Equation parseEquation(MultiBase*);
    std::string parseString(MultiBase*);
}

template<typename ValueType, ValueType(*parser)(MultiBase*)>
class Column
{
    public:
    static ValueType parse(MultiBase* dbValue){
        if(dbValue == nullptr){
            throw std::runtime_error("Attempted to parse non-existing value!");
        }
        return parser(dbValue);
    }
};

typedef Column<bool, parsers::parseBoolean> BooleanColumn;
typedef Column<uint32_t, parsers::parseHex> HexColumn;
typedef Column<Equation, parsers::parseEquation> EquationColumn;
typedef Column<std::string, parsers::parseString> StringColumn;
}