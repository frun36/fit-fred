#include"Environment.h"
#include"Parser/utility.h"

void EnvironmentFEE::emplace(EnvironmentFEE::Variable&& setting)
{
    m_settings.emplace(setting.name, setting);
}

void EnvironmentFEE::emplace(const EnvironmentFEE::Variable& setting)
{
    m_settings.emplace(setting.name, setting);
}

bool EnvironmentFEE::doesExist(const std::string& name)
{
    return m_settings.find(name) != m_settings.end();
}

double EnvironmentFEE::updateSetting(const std::string& name)
{
    Variable& setting = m_settings.at(name);
    std::vector<double> values;
    for(const auto& var: setting.equation.variables){
        values.emplace_back(m_settings.at(var).value);
    }
    setting.value = Utility::calculateEquation(setting.equation.equation, setting.equation.variables, values);
    return setting.value;
}

void EnvironmentFEE::setSettingValue(const std::string& name, double val)
{
    m_settings.at(name).value = val;
}

double EnvironmentFEE::getSetting(const std::string& name)
{
    return m_settings.at(name).value;
}