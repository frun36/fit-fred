#include<vector>
#include<string>
#include<list>
#include<unordered_map>
#include"Equation.h"

class Settings
{
public:    
    struct Setting{
        Setting(std::string name, Equation equation);
        std::string name;
        double value;
        Equation equation;
    };
    
    bool doesExist(const std::string& name);
    void emplace(const Setting&);
    void emplace(Setting&&);
    double updateSetting(const std::string& name);
    double getSetting(const std::string& name);

private:
    std::unordered_map<std::string, Setting> m_settings;
};