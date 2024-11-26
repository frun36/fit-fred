#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <optional>
#include <list>
#include "Equation.h"
#include "EnvironmentVariables.h"

class Board
{
   public:
    enum class Type { TCM,
                      PM };
    enum class Side { A,
                      C };

    struct Identity {
        Type type;
        std::optional<Side> side;
        uint8_t number;
    };

    struct ParameterInfo {
        // The encoding system requires serious rethinking, based on the electronics
        enum class ValueEncoding {
            Unsigned,
            Signed
        };
        enum class RefreshType {
            CNT,
            SYNC,
            NOT
        };

        ParameterInfo() = delete;
        ParameterInfo(const ParameterInfo& base, uint32_t boardAddress);
        ParameterInfo(
            std::string name,
            uint32_t baseAddress,
            uint32_t startBit,
            uint32_t bitLength,
            uint32_t regBlockSize,
            ValueEncoding valueEncoding,
            int64_t minValue,
            int64_t maxValue,
            Equation electronicToPhysic,
            Equation physicToElectronic,
            bool isFifo,
            bool isReadonly,
            RefreshType refreshType = RefreshType::NOT);

        const std::string name{ 0 };
        const uint32_t baseAddress{ 0 };
        const uint32_t startBit{ 0 };
        const uint32_t bitLength{ 0 };
        const size_t regBlockSize{ 1 };
        const ValueEncoding valueEncoding{ ValueEncoding::Unsigned };
        const int64_t minValue{ 0 };
        const int64_t maxValue{ 0 };

        Equation electronicToPhysic;
        Equation physicToElectronic;

        const bool isFifo{ false };
        const bool isReadonly{ false };

        const RefreshType refreshType;

        void storeValue(std::optional<double> physical, std::optional<int64_t> electronic)
        {
            m_physicalValue = physical;
            m_electronicValue = electronic;
        }

        double getPhysicalValue() const
        {
            if (!m_physicalValue.has_value()) {
                throw std::runtime_error(name + ": tried to access non-existing stored value");
            }
            return *m_physicalValue;
        }

        std::optional<double> getPhysicalValueOptional() const
        {
            return m_physicalValue;
        }

        int64_t getElectronicValue() const
        {
            if (!m_electronicValue.has_value()) {
                throw std::runtime_error(name + ": tried to access non-existing stored value");
            }
            return *m_electronicValue;
        }

        std::optional<int64_t> getElectronicValueOptional() const
        {
            return m_electronicValue;
        }

       private:
        std::optional<double> m_physicalValue;
        std::optional<int64_t> m_electronicValue;
    };

    Board(std::string name, uint32_t address, std::shared_ptr<Board> main = nullptr, std::shared_ptr<EnvironmentVariables> settings = nullptr);

    bool emplace(const ParameterInfo&);
    bool emplace(ParameterInfo&& info);

    ParameterInfo& operator[](const std::string&);
    ParameterInfo& operator[](std::string_view);
    const ParameterInfo& at(std::string_view) const;
    ParameterInfo& at(std::string_view);

    const std::unordered_map<std::string, ParameterInfo>& getParameters() const { return m_parameters; }
    bool doesExist(const std::string&);

    double getEnvironment(const std::string& variableName);
    void setEnvironment(const std::string& variableName, double value);
    void updateEnvironment(const std::string& variableName);

    double calculatePhysical(const std::string& param, int64_t electronic) const;
    int64_t parseElectronic(const std::string& param, uint32_t raw) const;

    int64_t calculateElectronic(const std::string& param, double physcial) const;
    uint32_t convertElectronicToRaw(const std::string& param, int64_t physcial) const;

    uint32_t getAddress() const { return m_address; }

    Type type() { return m_identity.type; }

    Identity getIdentity() const { return m_identity; }

    const std::string& getName() const { return m_name; }

    bool isTcm() const
    {
        return m_identity.type == Type::TCM;
    }

    std::shared_ptr<Board> getParentBoard()
    {
        return m_parentBoard;
    }

   private:
    Identity m_identity;
    std::string m_name;
    uint32_t m_address;
    std::shared_ptr<Board> m_parentBoard;
    std::shared_ptr<EnvironmentVariables> m_environmentalVariables;
    std::unordered_map<std::string, ParameterInfo> m_parameters;
};