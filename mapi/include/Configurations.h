#pragma once

#include "Mapi/mapi.h"
#include "Mapi/iterativemapi.h"
#include "Mapi/mapigroup.h"
#include "Database/databaseinterface.h"
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

       protected:
        virtual unordered_map<string, ConfigurationInfo>& getKnownConfigurations() = 0;

       public:
        void registerConfiguration(const string& name, vector<vector<MultiBase*>> dbData);
    };

    class PmConfigurations : public Mapi, public BoardConfigurations
    {
        unordered_map<string, ConfigurationInfo>& m_knownConfigurations;

        unordered_map<string, ConfigurationInfo>& getKnownConfigurations() override
        {
            return m_knownConfigurations;
        }

        string processInputMessage(string msg);
        string processOutputMessage(string msg);
    };

    class TcmConfigurations : public Iterativemapi, public BoardConfigurations
    {
        unordered_map<string, ConfigurationInfo> m_knownConfigurations;

        unordered_map<string, ConfigurationInfo>& getKnownConfigurations() override
        {
            return m_knownConfigurations;
        }

        optional<int16_t> m_currDelayA = nullopt;
        optional<int16_t> m_currDelayC = nullopt;
        int16_t m_delayDifference;
        const ConfigurationInfo* m_currCfg = nullptr;
        optional<ParsedResponse> m_delayResponse;

        enum class State { Idle,
                           ApplyDelays,
                           ApplyData } m_currState = State::Idle;

        static constexpr char* INTERNAL_PREFIX = "_INTERNAL:";

        optional<SwtSequence> processDelayInput(optional<int16_t> delayA, optional<int16_t> delayC);

        string processInputMessage(string msg) override;
        string processOutputMessage(string msg) override;
    };

   public:
    Configurations(Fred* fred, const unordered_map<string, Board>& boards);

   private:
    unordered_map<string, unique_ptr<BoardConfigurations>> m_boardCofigurationServices;
    unordered_map<string, vector<string>> m_boardsInConfigurations;
};
