#pragma once

#include "Fred/Mapi/mapi.h"
#include "Fred/Mapi/iterativemapi.h"
#include "Fred/Mapi/mapigroup.h"

#ifdef FIT_UNIT_TEST
    #include "databaseinterfaceMock.h"
#else
    #include "Database/databaseinterface.h"
#endif

#include "Parser/utility.h"
#include "unordered_map"
#include "SwtSequence.h"
#include "BasicRequestHandler.h"
#include <optional>
#include <cstring>
#include <memory>

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
   private:
    class BoardConfigurations : public BasicRequestHandler
    {
       public:
        struct ConfigurationInfo {
            const SwtSequence seq;
            const optional<int16_t> delayA;
            const optional<int16_t> delayC;

            ConfigurationInfo(const SwtSequence& seq, optional<int16_t> delayA, optional<int16_t> delayC) : seq(seq), delayA(delayA), delayC(delayC) {}
        };

        ConfigurationInfo getConfigurationInfo(const string& name) const;

        BoardConfigurations(std::shared_ptr<Board> board) : BasicRequestHandler(board) {}
    };

    class PmConfigurations : public Mapi, public BoardConfigurations
    {
       public:
        PmConfigurations(std::shared_ptr<Board> board) : BoardConfigurations(board) {}

        string processInputMessage(string msg);
        string processOutputMessage(string msg);
    };

    class TcmConfigurations : public Iterativemapi, public BoardConfigurations
    {
       public:
        TcmConfigurations(std::shared_ptr<Board> board) : BoardConfigurations(board) {}
        string processInputMessage(string msg) override;
        string processOutputMessage(string msg) override;

       private:
        optional<string> m_configurationName = nullopt;
        optional<ConfigurationInfo> m_configurationInfo = nullopt;
        enum class State { Idle,
                           ApplyingDelays,
                           DelaysApplied,
                           ApplyingData } m_state = State::Idle;

        // ControlServer stores delays as i16, and waits for (MAX_DELAY_DIFFERENCE + 10) milliseconds - which is odd, since the delay range is in nanoseconds
        optional<int16_t> m_delayA = nullopt;
        optional<int16_t> m_delayC = nullopt;
        int16_t m_delayDifference = 0;

        optional<string> m_delayResponse = nullopt;

        static constexpr const char* ContinueMessage = "_CONTINUE";

        optional<SwtSequence> processDelayInput(optional<int16_t> delayA, optional<int16_t> delayC);
        
        void reset()
        {
            m_configurationName = nullopt;
            m_configurationInfo = nullopt;
            m_delayA = 0;
            m_delayC = 0;
            m_delayDifference = 0;
            m_delayResponse = nullopt;
            m_state = State::Idle;
        }
    };

   public:
    Configurations(Fred* fred, const unordered_map<string, shared_ptr<Board>>& boards);

    string processInputMessage(string msg) override;

   private:
    unordered_map<string, unique_ptr<BoardConfigurations>> m_boardCofigurationServices;
    unordered_map<string, vector<string>> m_boardsInConfigurations;
};
