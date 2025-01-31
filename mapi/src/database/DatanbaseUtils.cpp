#include"DatabaseUtils.h"

namespace db_utils
{

namespace parsers
{
bool parseBoolean(MultiBase* field)
{
    return (field->getString() == "T");
}

uint32_t parseHex(MultiBase* field)
{
    std::stringstream ss;
    ss << field->getString();
    std::string hex(field->getString());
    uint32_t word = 0;
    ss >> std::hex >> word;
    return word;
}

Equation parseEquation(MultiBase* field)
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

std::string parseString(MultiBase* field)
{
    return field->getString();
}
}

}