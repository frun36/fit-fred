#include"database/Columns.h"

namespace db_fit
{
    
namespace parsers
{
bool booleanParser(MultiBase* data)
{
    return (data->getString() == "T");
}

uint32_t hexParser(MultiBase* data)
{
    std::stringstream ss;
    ss << data->getString();
    uint32_t word = 0;
    ss >> std::hex >> word;
    return word;
}

int64_t integerParser(MultiBase* data)
{
    return data->getDouble();
}

uint32_t unsignedParser(MultiBase* data)
{
    return data->getDouble(); 
}

std::string stringParser(MultiBase* data)
{
    return data->getString();
}

Equation equationParser(MultiBase* field)
{
    std::string equation = field->getString();
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
}
};
