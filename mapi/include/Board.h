#pragma once
#include<unordered_map>
#include<string>
#include<vector>
#include<memory>
#include<optional>
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

    ParameterInfo() = delete;
    ParameterInfo(
        std::string name, 
        uint32_t baseAddress, 
        uint8_t startBit, 
        uint8_t bitLength, 
        uint32_t regBlockSize, 
        ValueEncoding valueEncoding,
        double minValue,
        double maxValue,
        Equation rawToPhysic,
        Equation physicToRaw,
        bool isFifo,
        bool isReadonly
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
        throw std::out_of_range(name + ": value is out of allowed range");
        }
        m_value = value;
    }
    double getStoredValue() const {
        if(!m_value.has_value())
        {
            throw std::runtime_error(name + ": tried to access non-existing stored value");
        }
        return *m_value;
    }
    bool boundCheck(double value) const {return(value < minValue || value > maxValue);}

    private:
        std::optional<double> m_value;
    };

    Board(std::string name, uint32_t address);

    bool emplace(const ParameterInfo&);
    bool emplace(ParameterInfo&& info);

    ParameterInfo& operator[](const std::string&);
    ParameterInfo& at(const std::string&);
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