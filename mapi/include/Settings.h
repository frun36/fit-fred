#include<vector>
#include<string>
#include<list>
#include<unordered_map>

class Settings
{
public:    
    struct Equation{
        std::string equation;
        std::vector<std::string> variables;
        static Equation Empty() {return {"", std::vector<std::string>()};}
    };

    struct Setting{
        std::string name;
        double value;
        Equation equation;
    };
    
    bool doesExist(const std::string& name);
    void emplace(const Setting&);
    double updateSetting(const std::string& name);
    double getSetting(const std::string& name);

private:
    std::unordered_map<std::string, Setting> m_settings;
};