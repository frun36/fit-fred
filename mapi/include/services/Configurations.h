#pragma once

#include "BoardCommunicationHandler.h"
#include <optional>
#include <unordered_map>
#include <cstring>
#include <memory>
#include "services/BasicFitIndefiniteMapi.h"
#include "TCM.h"

#ifdef FIT_UNIT_TEST

#include "../../test/mocks/include/databaseinterface.h"
#include "../../test/mocks/include/mapi.h"
#include "../../test/mocks/include/utility.h"
#include "gtest/gtest.h"

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

*/

class Configurations : public Mapigroup
{
   public:
    class BoardConfigurations
    {
       public:
        struct ConfigurationInfo {
            string name;
            string req;
            optional<double> delayA;
            optional<double> delayC;

            ConfigurationInfo() = default;
            ConfigurationInfo(const string& name, const string& req, optional<double> delayA, optional<double> delayC) : name(name), req(req), delayA(delayA), delayC(delayC) {}
        };

        static vector<vector<MultiBase*>> fetchConfiguration(string_view configurationName, string_view boardName);
        static ConfigurationInfo parseConfigurationInfo(string_view configurationName, const vector<vector<MultiBase*>>& dbData);

        inline ConfigurationInfo getConfigurationInfo(string_view configurationName) const
        {
            auto dbData = fetchConfiguration(configurationName, m_boardName);
            return parseConfigurationInfo(configurationName, dbData);
        }

        // Due to lack of virtual inheritance for IndefiniteMapi, I don't think we can have BoardConfigurations inherit Mapi
        // And implement this method here
        virtual const string& getServiceName() const = 0;

        virtual ~BoardConfigurations() = default;
       protected:
        BoardCommunicationHandler m_handler;
        ConfigurationInfo m_configurationInfo;
        const string& m_boardName;

        BoardConfigurations(std::shared_ptr<Board> board) : m_handler(board), m_boardName(board->getName()) {}
    };

    class PmConfigurations : public Mapi, public BoardConfigurations
    {
       public:
        PmConfigurations(std::shared_ptr<Board> board) : BoardConfigurations(board) {}
        string processInputMessage(string msg);
        string processOutputMessage(string msg);

        const string& getServiceName() const override { return name; }
    };

    class TcmConfigurations : public BasicFitIndefiniteMapi, public BoardConfigurations
    {
       public:
        TcmConfigurations(std::shared_ptr<Board> board) : BoardConfigurations(board)
        {
            if (!board->doesExist(string(tcm_parameters::DelayA)) || !board->doesExist(string(tcm_parameters::DelayC)))
                throw runtime_error("Couldn't construct TcmConfigurations: no delay parameters");
        }

        void processExecution() override;

       private:
        string m_response;

        bool handleDelays();
        bool handleData();
        void handleResetErrors();

        const string& getServiceName() const override { return name; }
    };

   public:
    Configurations() = default;
    Configurations(const string& fredName, const unordered_map<string, shared_ptr<Board>>& boards);

    vector<string> fetchBoardNamesToConfigure(const string& configurationName) const;

    string processInputMessage(string msg) override;
    string processOutputMessage(string msg) override { throw std::runtime_error("Configurations: unexpectedly received '" + msg + "' from ALF"); };

    const unordered_map<string, unique_ptr<BoardConfigurations>>& getBoardConfigurationServices() const { return m_boardCofigurationServices; }

   private:
    unordered_map<string, unique_ptr<BoardConfigurations>> m_boardCofigurationServices;
    string m_fredName;
};
