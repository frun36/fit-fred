#include<unordered_map>
#include<string>
#include<memory>
#include"ParameterInfo.h"

class Board
{
public:
    Board(std::string name, uint32_t address);

    std::pair<std::unordered_map<std::string, ParameterInfo>::iterator, bool> emplace(ParameterInfo&&);
    std::pair<std::unordered_map<std::string, ParameterInfo>::iterator, bool> emplace(const ParameterInfo&);

    ParameterInfo& operator[](const std::string&);
    ParameterInfo& get(const std::string&);
    bool doesExist(const std::string&);

    double calculatePhysical(const std::string& param, uint32_t raw);
    uint32_t calculateRaw(const std::string& param, double physcial);
    uint64_t calculateRaw64(const std::string& param, double physical);

    uint32_t getAddress() const {return m_address;}

private:
    std::string m_name;
    uint32_t m_address;   
    std::shared_ptr<Board> m_mainBoard;
    std::unordered_map<std::string, ParameterInfo> m_parameters;
};