#pragma once

#include "BoardCommunicationHandler.h"
#include <optional>
#include <unordered_map>
#include <cstring>
#include <memory>

#ifdef FIT_UNIT_TEST

#include "../../test/mocks/include/databaseinterface.h"
#include "../../test/mocks/include/mapi.h"
#include "../../test/mocks/include/utility.h"
#include "gtest/gtest.h"

namespace
{
class ConfigurationsTest_Delays_Test;
class ConfigurationsTest_PmPim_Test;
class ConfigurationsTest_Tcm_Test;
} // namespace

#else

#include "Parser/utility.h"
#include "Database/databaseinterface.h"
#include "Fred/Mapi/mapi.h"
#include "Fred/Mapi/indefinitemapi.h"
#include "Fred/Mapi/mapigroup.h"

#endif

/*
Control Server - void fileRead() from FITelectronics.h
TCM - parsing of group "TCM":
 - struct TypeTCM::Settings set - specific registers for which configurations are applied
 - file parsed into QMap<quint8, quint32> M (<addr, val>)
 - TCM.set updated with correct values
 - all addresses < 256
TCM - if(doApply)
 - QHash TCMparameters <name, {addr, width, shift}>
 - Generation of IPbus transaction, with DELAY_A & DELAY_C (if changed)
 ! wait for max(|CURR_DELAY_A - CURR_DELAY_A|, |CURR_DELAY_C - NEW_DELAY_C|) + 10ms after transceive
 ! remove COUNTERS_UPD_RATE - will be applied afterwards
 - New IPbus transaction generation
 ! 0xE - RMW to not change bits 4..7 and 10 (hist. control), only write 0..3 and 8..9
 - Add remaining params, transceive
 ! if we changed CH_MASK_A or CH_MASK_C - sleep 10 ms
 ! apply_RESET_ERRORS() - *
 ! apply_COUNTERS_UPD_RATE() if reg50 changed - **

For each PM - parsing
 - for each of TypePM allPMs[20]
 - pm->set - update settings value, create QMap M

For PMs enabled in PM_MASK_SPI and if (doApply)
 - generate packet, update value if it changed


* apply_RESET_ERRORS()
for TCM and each PM
 - GBTunit::controlAddress = 0xD8
 - RMWbits: 0xFFFF00FF, 0x00000000 (clear all reset bits)
 - RMWbits: 0xFFFFFFFF, set: readoutFSM (14), GBTRxError (11), errorReport (15)
 - RMWbits: 0xFFBF00FF, 0x00000000 (clear all reset bits and unlock)
then write 0x4 to 0xF

** apply_COUNTERS_UPD_RATE()
 - write 0 to 0x50 (counters read interval)
 - emptyCountBuffers();
 - readCountersDirectly();
 - write chosen value to 0x50 if in range - 0: no update; 1: 0.1s; 2: 0.2s, 3: 0.5s, 4: 1s; 5: 2s; 6: 5s; 7: 10s
If everything was successful, create log entry


Configurations DB structure:
CONFIGURATIONS:
CONFIGURATION_NAME
BOARD_NAME
PARAMETER_ID
PARAMETER_VALUE

*/

class Configurations : public Mapigroup
{
#ifdef FIT_UNIT_TEST
    FRIEND_TEST(::ConfigurationsTest, Delays);
    FRIEND_TEST(::ConfigurationsTest, PmPim);
    FRIEND_TEST(::ConfigurationsTest, Tcm);
#endif

   public:
    class BoardConfigurations
    {
       public:
        struct ConfigurationInfo {
            const string req;
            const optional<double> delayA;
            const optional<double> delayC;

            ConfigurationInfo(const string& req, optional<double> delayA, optional<double> delayC) : req(req), delayA(delayA), delayC(delayC) {}
        };

        static vector<vector<MultiBase*>> fetchConfiguration(string_view configuration, string_view board);
        static ConfigurationInfo getConfigurationInfo(string_view configurationName, const vector<vector<MultiBase*>>& dbData);

        virtual string_view getBoardName() const = 0;

        inline ConfigurationInfo fetchAndGetConfigurationInfo(string_view name) const
        {
            auto dbData = fetchConfiguration(name, getBoardName());
            return getConfigurationInfo(name, dbData);
        }

        virtual ~BoardConfigurations() = default;
    };

   private:
    class PmConfigurations : public Mapi, public BoardConfigurations
    {
#ifdef FIT_UNIT_TEST
        FRIEND_TEST(::ConfigurationsTest, Delays);
        FRIEND_TEST(::ConfigurationsTest, PmPim);
#endif
       private:
        BoardCommunicationHandler m_pm;

       public:
        PmConfigurations(std::shared_ptr<Board> board) : m_pm(board) {}

        string_view getBoardName() const override { return m_pm.getBoard()->getName(); }

        string processInputMessage(string msg);
        string processOutputMessage(string msg);
    };

    class TcmConfigurations : public IndefiniteMapi, public BoardConfigurations
    {
#ifdef FIT_UNIT_TEST
        FRIEND_TEST(::ConfigurationsTest, Delays);
        FRIEND_TEST(::ConfigurationsTest, Tcm);
#endif
       public:
        TcmConfigurations(std::shared_ptr<Board> board) : m_tcm(board)
        {
            if (!board->doesExist("DELAY_A") || !board->doesExist("DELAY_C"))
                throw runtime_error("Couldn't construct TcmConfigurations: no delay parameters");
        }

       private:
        BoardCommunicationHandler m_tcm;

        string_view getBoardName() const override { return m_tcm.getBoard()->getName(); }

        optional<string> m_configurationName = nullopt;
        optional<ConfigurationInfo> m_configurationInfo = nullopt;

        double m_delayDifference = 0;

        optional<double> getDelayA() const
        {
            return m_tcm.getBoard()->at("DELAY_A").getPhysicalValueOptional();
        }

        optional<double> getDelayC() const
        {
            return m_tcm.getBoard()->at("DELAY_C").getPhysicalValueOptional();
        }

        optional<SwtSequence> processDelayInput(optional<double> delayA, optional<double> delayC);

        bool handleDelays(string& response);
        bool handleData(string& response);
        void handleResetErrors();

        void processExecution() override;

        void reset()
        {
            m_configurationName = nullopt;
            m_configurationInfo = nullopt;
            m_delayDifference = 0;
        }
    };

   public:
    Configurations() = default;
    Configurations(const string& fredName, const unordered_map<string, shared_ptr<Board>>& boards);

    string processInputMessage(string msg) override;
    string processOutputMessage(string msg) override { throw std::runtime_error("Configurations: unexpectedly received '" + msg + "' from ALF"); };

    const unordered_map<string, unique_ptr<BoardConfigurations>>& getBoardConfigurationServices() const { return m_boardCofigurationServices; }

   private:
    unordered_map<string, unique_ptr<BoardConfigurations>> m_boardCofigurationServices;
    string m_fredName;
};
