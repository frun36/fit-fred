#pragma once

#include<vector>
#include<string>
#include<list>
#include<unordered_map>
#include"Equation.h"

class EnvironmentFEE
{
public:    
    struct Variable{
        Variable(std::string name_, Equation equation_): 
            name(name_), equation(equation_)
        {

        }

        std::string name;
        double value;
        Equation equation;
    };
    
    bool doesExist(const std::string& name);
    void emplace(const Variable&);
    void emplace(Variable&&);
    double updateVariable(const std::string& name);
    void setVariable(const std::string& name, double val);
    double getVariable(const std::string& name);

private:
    std::unordered_map<std::string, Variable> m_settings;
};