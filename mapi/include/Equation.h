#pragma once

#include <string>
#include <vector>
#include<cstdint>

using namespace std;

#include<map>
#include <cmath>

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <climits>
#include <cmath>

struct Equation {
    std::string equation;
    std::vector<std::string> variables;
    static Equation Empty() { return { "", std::vector<std::string>() }; }
    static double calculate(std::string equation, const std::vector<std::string>& variables, const std::vector<double>& values);
};