#include "board/EnvironmentVariables.h"

void EnvironmentVariables::emplace(EnvironmentVariables::Variable&& setting)
{
    m_settings.emplace(setting.name, setting);
}

void EnvironmentVariables::emplace(const EnvironmentVariables::Variable& setting)
{
    m_settings.emplace(setting.name, setting);
}

bool EnvironmentVariables::doesExist(const std::string& name)
{
    return m_settings.find(name) != m_settings.end();
}

double EnvironmentVariables::updateVariable(const std::string& name)
{
    Variable& setting = m_settings.at(name);
    std::vector<double> values;
    for (const auto& var : setting.equation.variables) {
        values.emplace_back(m_settings.at(var).value);
    }
    setting.value = Equation::calculate(setting.equation.equation, setting.equation.variables, values);
    return setting.value;
}

void EnvironmentVariables::setVariable(const std::string& name, double val)
{
    m_settings.at(name).value = val;
}

double EnvironmentVariables::getVariable(const std::string& name)
{
    return m_settings.at(name).value;
}
