#include "utils/Equation.h"
#include <algorithm> // necessary for Parser/calculator.h to compile
#include "Parser/calculator.h"
#include "Alfred/print.h"

double Equation::calculate(std::string equation, const std::vector<std::string>& variables, const std::vector<double>& values)
{
    std::map<std::string, int64_t> varMap;

    for (size_t i = 0; i < values.size(); i++) {
        if (trunc(values[i]) == values[i]) {
            varMap[variables[i]] = int64_t(values[i]);
        } else {
            size_t pos = -1;
            while ((pos = equation.find(variables[i], pos + 1)) != std::string::npos) {
                equation.insert(pos + variables[i].size(), "/1000");
            }
            varMap[variables[i]] = int64_t(values[i] * 1000);
        }
    }

    try {
        return (calculator::eval(equation, varMap));
    } catch (const calculator::error& err) {
        Print::PrintError("Cannot parse equation - " + equation);
    }

    return 0;
}
