#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>
#include<vector>
// #include <Database/databaseinterface.h>

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
        throw std::out_of_range("Value is out of allowed range.");
        }
        m_value = value;
    }
    double getStoredValue() const {return m_value;}

private:
    double m_value;
};