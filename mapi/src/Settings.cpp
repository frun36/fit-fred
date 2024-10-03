#include"Settings.h"
#include"Parser/utility.h"

void Settings::emplace(Settings::Setting&& setting)
{
    m_settings.emplace(setting.name, setting);
}

void Settings::emplace(const Settings::Setting& setting)
{
    m_settings.emplace(setting.name, setting);
}

bool Settings::doesExist(const std::string& name)
{
    return m_settings.find(name) != m_settings.end();
}

double Settings::updateSetting(const std::string& name)
{
    Setting& setting = m_settings.at(name);
    std::vector<double> values;
    for(const auto& var: setting.equation.variables){
        values.emplace_back(m_settings.at(var).value);
    }
    setting.value = Utility::calculateEquation(setting.equation.equation, setting.equation.variables, values);
    return setting.value;
}

void Settings::setSettingValue(const std::string& name, double val)
{
    m_settings.at(name).value = val;
}

double Settings::getSetting(const std::string& name)
{
    return m_settings.at(name).value;
}