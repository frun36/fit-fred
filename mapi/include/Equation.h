#pragma once
#include<string>
#include<vector>

struct Equation{
    std::string equation;
    std::vector<std::string> variables;
    static Equation Empty() {return {"", std::vector<std::string>()};}
};