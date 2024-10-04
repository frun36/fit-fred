#pragma once
#include<vector>
#include<string>
#include<list>
#include<unordered_map>
#include"Equation.h"

class Environment
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
    double updateSetting(const std::string& name);
    void setSettingValue(const std::string& name, double val);
    double getSetting(const std::string& name);

private:
    std::unordered_map<std::string, Variable> m_settings;
};