#include"Settings.h"
#include"Parser/utility.h"

void Environment::emplace(Environment::Variable&& setting)
{
    m_settings.emplace(setting.name, setting);
}

void Environment::emplace(const Environment::Variable& setting)
{
    m_settings.emplace(setting.name, setting);
}

bool Environment::doesExist(const std::string& name)
{
    return m_settings.find(name) != m_settings.end();
}

double Environment::updateSetting(const std::string& name)
{
    Variable& setting = m_settings.at(name);
    std::vector<double> values;
    for(const auto& var: setting.equation.variables){
        values.emplace_back(m_settings.at(var).value);
    }
    setting.value = Utility::calculateEquation(setting.equation.equation, setting.equation.variables, values);
    return setting.value;
}

void Environment::setSettingValue(const std::string& name, double val)
{
    m_settings.at(name).value = val;
}

double Environment::getSetting(const std::string& name)
{
    return m_settings.at(name).value;
}