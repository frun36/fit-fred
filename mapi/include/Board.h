#pragma once
#include<unordered_map>
#include<string>
#include<vector>
#include<cstdint>
#include<memory>
#include<optional>
#include<list>
#include"Equation.h"
#include"Environment.h"


class Board
{
public:
    enum class Type {TCM, PM};

    struct ParameterInfo {
    // The encoding system requires serious rethinking, based on the electronics
    enum class ValueEncoding {
        Unsigned, Signed
    };
    enum class RefreshType {
        CNT, SYNC, NOT
    };

    ParameterInfo() = delete;
    ParameterInfo(const ParameterInfo& base, uint32_t boardAddress);
    ParameterInfo(
        std::string name, 
        uint32_t baseAddress, 
        uint8_t startBit, 
        uint8_t bitLength, 
        uint32_t regBlockSize, 
        ValueEncoding valueEncoding,
        double minValue,
        double maxValue,
        Equation electronicToPhysic,
        Equation physicToElectronic,
        bool isFifo,
        bool isReadonly,
        RefreshType refreshType = RefreshType::NOT
    ) ;

        const std::string name{0};
        const uint32_t baseAddress{0};
        const uint8_t startBit{0};
        const uint8_t bitLength{0};
        const size_t regBlockSize{1};
        const ValueEncoding valueEncoding{ValueEncoding::Unsigned};
        const double minValue{0};
        const double maxValue{0};

    Equation electronicToPhysic;
    Equation physicToElectronic;
    
    const bool isFifo{false};
    const bool isReadonly{false};

    const RefreshType refreshType;

    void storeValue(double value)
    {
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

    Board(std::string name, uint32_t address, std::shared_ptr<Board> main=nullptr, std::shared_ptr<EnvironmentFEE> settings=nullptr);

    bool emplace(const ParameterInfo&);
    bool emplace(ParameterInfo&& info);

    ParameterInfo& operator[](const std::string&);
    ParameterInfo& operator[](std::string_view);
    ParameterInfo& at(const std::string&);
    ParameterInfo& at(std::string_view);

    const std::unordered_map<std::string, ParameterInfo>& getParameters() const {return m_parameters;}
    bool doesExist(const std::string&);

    double getEnvironment(const std::string& variableName);
    void setEnvironment(const std::string& variableName, double value);
    void updateEnvironment(const std::string& variableName);

    double calculatePhysical(const std::string& param, uint32_t raw);
    uint32_t calculateRaw(const std::string& param, double physcial);

    uint32_t getAddress() const {return m_address;}

    Type type() { return m_boardType; }
    void setType(Type type) { m_boardType = type;}

    const std::string& getName() const { return m_name; }

private:
    Type m_boardType;
    std::string m_name;
    uint32_t m_address;   
    std::shared_ptr<Board> m_mainBoard;
    std::shared_ptr<EnvironmentFEE> m_settings;
    std::unordered_map<std::string, ParameterInfo> m_parameters;
};