#pragma once
#include<unordered_map>
#include<string>
#include<vector>
#include<memory>

class Board
{
public:
    struct ParameterInfo {
    // The encoding system requires serious rethinking, based on the electronics
    enum class ValueEncoding {
        Unsigned, Signed
    };

    struct Equation{
        std::string equation;
        std::vector<std::string> variables;
    };

    ParameterInfo() = default;
    ParameterInfo(
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
    ) ;

    const std::string name{0};
    const uint32_t baseAddress{0};
    const uint8_t startBit{0};
    const uint8_t bitLength{0};
    const size_t regBlockSize{1};
    const ValueEncoding valueEncoding{ValueEncoding::Unsigned};
    const double minValue{0};
    const double maxValue{0};

    Equation rawToPhysic;
    Equation physicToRaw;
    
    const bool isFifo{false};
    const bool isReadonly{false};

    void storeValue(double value)
    {
        if (value < minValue || value > maxValue) {
        throw std::out_of_range(name + "Value is out of allowed range.");
        }
        m_value = value;
    }
    double getStoredValue() const {return m_value;}
    bool boundCheck(double value) const {return(value < minValue || value > maxValue);}

    private:
        double m_value;
    };

    Board(std::string name, uint32_t address);

    bool emplace(const ParameterInfo&);

    ParameterInfo& operator[](const std::string&);
    ParameterInfo& get(const std::string&);
    bool doesExist(const std::string&);

    double calculatePhysical(const std::string& param, uint32_t raw);
    double calculatePhysical64(const std::string& param, uint64_t raw);
    
    uint32_t calculateRaw(const std::string& param, double physcial);
    uint64_t calculateRaw64(const std::string& param, double physical);

    uint32_t getAddress() const {return m_address;}

private:
    std::string m_name;
    uint32_t m_address;   
    std::shared_ptr<Board> m_mainBoard;
    std::unordered_map<std::string, ParameterInfo> m_parameters;
};