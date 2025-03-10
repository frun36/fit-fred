#pragma once

#include <string>
#include <unordered_map>
#include <string_view>
#include "utils/Equation.h"

class EnvironmentVariables
{
   public:
    struct Variable {
        Variable(std::string name_, Equation equation_) : name(name_), equation(equation_)
        {
        }

        std::string name;
        double value;
        Equation equation;
    };

    bool doesExist(const std::string& name);
    void emplace(const Variable&);
    void emplace(Variable&&);
    double updateVariable(const std::string& name);
    void setVariable(const std::string& name, double val);
    double getVariable(const std::string& name);

   private:
    std::unordered_map<std::string, Variable> m_settings;
};

namespace environment
{

namespace parameters
{
constexpr std::string_view ExtenalClock{ "LHC_CLOCK" };
constexpr std::string_view InternalClock{ "INTERNAL_CLOCK" };
constexpr std::string_view SystemClock{ "SYSTEM_CLOCK" };
constexpr std::string_view TDC{ "TDC" };
constexpr std::string_view PmA0BoardId{ "PMA0_BOARD_ID" };
constexpr std::string_view PmC0BoardId{ "PMC0_BOARD_ID" };
constexpr std::string_view TcmBoardId{ "TCM_BOARD_ID" };
constexpr std::string_view SystemId{ "SYSTEM_ID" };
constexpr std::string_view BcIdOffsetDefault{ "BCID_OFFSET_DEFAULT" };
constexpr std::string_view Trigger1Signature{ "TRIGGER_1_SIGNATURE" };
constexpr std::string_view Trigger2Signature{ "TRIGGER_2_SIGNATURE" };
constexpr std::string_view Trigger3Signature{ "TRIGGER_3_SIGNATURE" };
constexpr std::string_view Trigger4Signature{ "TRIGGER_4_SIGNATURE" };
constexpr std::string_view Trigger5Signature{ "TRIGGER_5_SIGNATURE" };
constexpr std::string_view BcInvterval{ "BC_INTERVAL" };
} // namespace parameters

namespace constants
{
constexpr bool SourceInternalClock = false;
constexpr bool SourceExternalClock = true;
} // namespace constants

} // namespace environment
