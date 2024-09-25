#include<stdexcept>
#include"Board.h"
#include"utils.h"
#include"Parser/utility.h"

Board::Board(std::string name, uint32_t address): m_name(name), m_address(address), m_mainBoard(nullptr)
{

}

Board::ParameterInfo::ParameterInfo(
        std::string name_, 
        uint32_t baseAddress_, 
        uint8_t startBit_, 
        uint8_t bitLength_, 
        uint32_t regBlockSize_, 
        ValueEncoding valueEncoding_,
        double minValue_,
        double maxValue_,
        Equation rawToPhysic_,
        Equation physicToRaw_,
        bool isFifo_,
        bool isReadonly_
    ) : 
        name(name_), 
        baseAddress(baseAddress_), 
        startBit(startBit_), 
        bitLength(bitLength_), 
        regBlockSize(regBlockSize_),
        valueEncoding(valueEncoding_),
        minValue(minValue_),
        maxValue(maxValue_),
        rawToPhysic(rawToPhysic_),
        physicToRaw(physicToRaw_),
        isFifo(isFifo_),
        isReadonly(isReadonly_),
        m_value(0.0) {}




bool Board::emplace(const ParameterInfo& info)
{
    if(info.baseAddress < m_address)
    {
        throw std::runtime_error("Attempt to add " + info.name + " to board " + m_name + " failer, address lower than board base address");
    }
    return m_parameters.emplace(info.name, info).second;
}

Board::ParameterInfo& Board::operator[](const std::string& param)
{
    return at(param);
}

Board::ParameterInfo& Board::at(const std::string& param)
{
    return m_parameters.at(param);
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

    ParameterInfo& info = m_parameters.at(param);
    int64_t decoded = 0;

    if(info.valueEncoding == ParameterInfo::ValueEncoding::Unsigned){
        decoded = static_cast<int64_t>(raw);
    }
    else{
        decoded = twosComplementDecode<int32_t>(getBitField(raw, info.startBit, info.bitLength), info.bitLength);
    }

    if(info.rawToPhysic.equation == "")
    {
        return decoded;
    }

    std::vector<double> values;
    for(const auto& var: info.rawToPhysic.variables)
    {
        if(var == info.name) 
        {
            values.emplace_back(decoded);
        }
        else if(m_parameters.find(var) != m_parameters.end())
        {
            values.emplace_back(m_parameters.at(var).getStoredValue());
        }
        else if(m_mainBoard != nullptr && m_mainBoard->doesExist(var))
        {
            values.emplace_back(m_mainBoard->at(var).getStoredValue());
        }
        else
        {
            throw std::runtime_error("Parameter " + var + " does not exist!");
        }
    }
    if(values.size() !=  info.rawToPhysic.variables.size())
    {
        throw std::runtime_error("Parameter " + param + ": parsing equation failed!");
    }
    return Utility::calculateEquation(info.rawToPhysic.equation, info.rawToPhysic.variables, values);
}

uint32_t Board::calculateRaw(const std::string& param, double physical) 
{
    if(m_parameters.find(param) == m_parameters.end())
    {
        throw std::runtime_error(param + " does not exist!");
    }

    ParameterInfo& info = m_parameters.at(param);

    if(info.physicToRaw.equation == "")
    {
        if(info.valueEncoding != ParameterInfo::ValueEncoding::Unsigned){
            return twosComplementEncode(static_cast<int32_t>(physical), info.bitLength) << info.startBit;
        }
        return static_cast<uint32_t>(physical) << info.startBit;
    }

    std::vector<double> values;
    for(const auto& var: info.rawToPhysic.variables)
    {
        if(var == info.name) 
        {
            values.emplace_back(physical);
            continue;
        }
        else if(m_parameters.find(var) != m_parameters.end())
        {
            values.emplace_back(m_parameters.at(var).getStoredValue());
        }
        else if(m_mainBoard != nullptr && m_mainBoard->doesExist(var))
        {
            values.emplace_back(m_mainBoard->at(var).getStoredValue());
        }
        else
        {
            throw std::runtime_error("Parameter " + var + " does not exist!");
        }
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

    ParameterInfo& info = m_parameters.at(param);
    if(info.physicToRaw.equation == "")
    {
        if(info.valueEncoding != ParameterInfo::ValueEncoding::Unsigned){
            return twosComplementEncode<int64_t, uint64_t>(static_cast<int64_t>(physical), info.bitLength) << info.startBit;
        }
        return static_cast<uint64_t>(physical) << info.startBit;
    }

    std::vector<double> values;
    for(const auto& var: info.rawToPhysic.variables)
    {
        if(var == info.name) 
        {
            values.emplace_back(physical);
            continue;
        }
        else if(m_parameters.find(var) != m_parameters.end())
        {
            values.emplace_back(m_parameters.at(var).getStoredValue());
        }
        else if(m_mainBoard != nullptr && m_mainBoard->doesExist(var))
        {
            values.emplace_back(m_mainBoard->at(var).getStoredValue());
        }
        else
        {
            throw std::runtime_error("Parameter " + var + " does not exist!");
        }
    }
    int64_t calculated = static_cast<int64_t>(Utility::calculateEquation(info.rawToPhysic.equation, info.rawToPhysic.variables, values));
    
    if(info.valueEncoding != ParameterInfo::ValueEncoding::Unsigned){
        return twosComplementEncode<int64_t, uint64_t>(calculated, info.bitLength) << info.startBit;
    }
    
    return static_cast<uint64_t>(calculated) << info.startBit;
}

double Board::calculatePhysical64(const std::string& param, uint64_t raw) 
{
    if(m_parameters.find(param) == m_parameters.end())
    {
        throw std::runtime_error(param + " does not exist!");
    }

    ParameterInfo& info = m_parameters.at(param);
    int64_t decoded = 0;

    if(info.valueEncoding == ParameterInfo::ValueEncoding::Unsigned){
        decoded = static_cast<int64_t>(raw);
    }
    else{
        decoded = twosComplementDecode<int64_t>(getBitField(raw, info.startBit, info.bitLength), info.bitLength);
    }

    if(info.rawToPhysic.equation == "")
    {
        return decoded;
    }

    std::vector<double> values;
    for(const auto& var: info.rawToPhysic.variables)
    {
        if(var == info.name) 
        {
            values.emplace_back(decoded);
            continue;
        }
        else if(m_parameters.find(var) != m_parameters.end())
        {
            values.emplace_back(m_parameters.at(var).getStoredValue());
        }
        else if(m_mainBoard != nullptr && m_mainBoard->doesExist(var))
        {
            values.emplace_back(m_mainBoard->at(var).getStoredValue());
        }
        else
        {
            throw std::runtime_error("Parameter " + var + " does not exist!");
        }
    }

    return Utility::calculateEquation(info.rawToPhysic.equation, info.rawToPhysic.variables, values);
}