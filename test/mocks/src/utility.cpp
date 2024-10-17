#include "../include/utility.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <map>
#include <cmath>
#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

#include "Parser/calculator.h"

#pragma GCC diagnostic push

vector<string> Utility::splitString(const string& text, string by)
{
    vector<string> result;
    string temp = text;

    while (temp.size()) {
        size_t index = temp.find(by);
        if (index != string::npos) {
            result.push_back(temp.substr(0, index));
            temp = temp.substr(index + by.length());
        } else {
            result.push_back(temp);
            temp = "";
            break;
        }
    }

    return result;
}

void Utility::removeWhiteSpaces(string& text)
{
    text.erase(remove_if(text.begin(), text.end(), [](unsigned char c) { return isspace(c); }), text.end());
    text.erase(remove_if(text.begin(), text.end(), [](unsigned char c) { return iscntrl(c); }), text.end());
}

double Utility::calculateEquation(string& equation, vector<string>& variables, vector<double>& values)
{
    map<string, int64_t> varMap;

    for (size_t i = 0; i < values.size(); i++) {
        if (trunc(values[i]) == values[i]) {
            varMap[variables[i]] = int64_t(values[i]);
        } else {
            size_t pos = -1;
            while ((pos = equation.find(variables[i], pos + 1) != string::npos)) {
                equation.insert(pos + variables[i].size(), "/1000");
            }
            varMap[variables[i]] = int64_t(values[i] * 1000);
        }
    }

    try {
        return (calculator::eval(equation, varMap));
    } catch (const calculator::error& err) {
        cerr << "Cannot parse equation!";
    }

    return 0;
}