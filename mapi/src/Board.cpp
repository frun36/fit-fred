#include<stdexcept>
#include"Board.h"
#include"utils.h"
#include"Parser/utility.h"

Board::Board(std::string name, uint32_t address): m_name(name), m_address(address)
{

}

std::pair<std::unordered_map<std::string, ParameterInfo>::iterator, bool> Board::emplace(ParameterInfo&& info)
{
    if(info.baseAddress < m_address)
    {
        throw std::runtime_error("Attempt to add " + info.name + " to board " + m_name + " failer, address lower than board base address");
    }
    return m_parameters.emplace(info.name, info);
}

std::pair<std::unordered_map<std::string, ParameterInfo>::iterator, bool> Board::emplace(const ParameterInfo& info)
{
    if(info.baseAddress < m_address)
    {
        throw std::runtime_error("Attempt to add " + info.name + " to board " + m_name + " failer, address lower than board base address");
    }
    return m_parameters.emplace(info.name, info);
}

ParameterInfo& Board::operator[](const std::string& param)
{
    return get(param);
}

ParameterInfo& Board::get(const std::string& param)
{
    auto itr = m_parameters.find(param);
    if(itr == m_parameters.end())
    {
        throw std::runtime_error(param + " does not exist!");   
    }
    return itr->second;
}

bool Board::doesExist(const std::string& param)
{
    return m_parameters.find(param) != m_parameters.end();
}

double Board::calculatePhysical(const std::string& param, uint32_t raw) 
{
    if(m_parameters.find(param) == m_parameters.end())
    {
        throw std::runtime_error(param + " does not exist!");
    }

    ParameterInfo& info = m_parameters[param];
    int64_t decoded = 0;

    if(info.valueEncoding == ParameterInfo::ValueEncoding::Unsigned){
        decoded = raw;
    }
    else{
        decoded = twosComplementDecode<int64_t>(getBitField(raw, info.startBit, info.bitLength), info.bitLength);
    }

    std::vector<double> values;
    for(const auto& var: info.rawToPhysic.variables)
    {
        if(var == info.name) 
        {
            values.emplace_back(decoded);
            continue;
        }
        if(m_parameters.find(var) != m_parameters.end())
        {
            values.emplace_back(m_parameters[var].getStoredValue());
        }
        if(m_mainBoard != nullptr && m_mainBoard->doesExist(param))
        {
            values.emplace_back(m_mainBoard->get(var).getStoredValue());
        }
        throw std::runtime_error("Parameter " + var + " does not exist!");
    }

    return Utility::calculateEquation(info.rawToPhysic.equation, info.rawToPhysic.variables, values);
}

uint32_t Board::calculateRaw(const std::string& param, double physical) 
{
    if(m_parameters.find(param) == m_parameters.end())
    {
        throw std::runtime_error(param + " does not exist!");
    }

    ParameterInfo& info = m_parameters[param];

    std::vector<double> values;
    for(const auto& var: info.rawToPhysic.variables)
    {
        if(var == info.name) 
        {
            values.emplace_back(physical);
            continue;
        }
        if(m_parameters.find(var) != m_parameters.end())
        {
            values.emplace_back(m_parameters[var].getStoredValue());
        }
        if(m_mainBoard != nullptr && m_mainBoard->doesExist(param))
        {
            values.emplace_back(m_mainBoard->get(var).getStoredValue());
        }
        throw std::runtime_error("Parameter " + var + " does not exist!");
    }
    int32_t calculated = static_cast<int32_t>(Utility::calculateEquation(info.rawToPhysic.equation, info.rawToPhysic.variables, values));
    
    if(info.valueEncoding != ParameterInfo::ValueEncoding::Unsigned){
        return twosComplementEncode(calculated, info.bitLength) << info.startBit;
    }
    
    return static_cast<uint32_t>(calculated) << info.startBit;
}

uint64_t Board::calculateRaw64(const std::string& param, double physical)
{
    if(m_parameters.find(param) == m_parameters.end())
    {
        throw std::runtime_error(param + " does not exist!");
    }

    ParameterInfo& info = m_parameters[param];

    std::vector<double> values;
    for(const auto& var: info.rawToPhysic.variables)
    {
        if(var == info.name) 
        {
            values.emplace_back(physical);
            continue;
        }
        if(m_parameters.find(var) != m_parameters.end())
        {
            values.emplace_back(m_parameters[var].getStoredValue());
        }
        if(m_mainBoard != nullptr && m_mainBoard->doesExist(param))
        {
            values.emplace_back(m_mainBoard->get(var).getStoredValue());
        }
        throw std::runtime_error("Parameter " + var + " does not exist!");
    }
    int64_t calculated = static_cast<int64_t>(Utility::calculateEquation(info.rawToPhysic.equation, info.rawToPhysic.variables, values));
    
    if(info.valueEncoding != ParameterInfo::ValueEncoding::Unsigned){
        return twosComplementEncode(calculated, info.bitLength) << info.startBit;
    }
    
    return static_cast<uint64_t>(calculated) << info.startBit;
}