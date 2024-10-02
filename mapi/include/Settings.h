#include<vector>
#include<string>
#include<list>
#include<unordered_map>
#include"Equation.h"

class Settings
{
public:    
    struct Setting{
        Setting(std::string name_, Equation equation_): 
            name(name_), equation(equation_)
        {

        }

        std::string name;
        double value;
        Equation equation;
    };
    
    bool doesExist(const std::string& name);
    void emplace(const Setting&);
    void emplace(Setting&&);
    double updateSetting(const std::string& name);
    void setSettingValue(const std::string& name, double val);
    double getSetting(const std::string& name);

private:
    std::unordered_map<std::string, Setting> m_settings;
};