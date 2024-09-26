#pragma once

#include "Mapi/iterativemapi.h"
#include "Parser/utility.h"
#include "unordered_map"
#include "SwtSequence.h"
#include <optional>

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
 ! apply_RESET_ERRORS() - ?
 ! apply_COUNTERS_UPD_RATE() if reg50 changed - ?

For each PM - parsing
 - for each of TypePM allPMs[20]
 - pm->set - update settings value, create QMap M

For PMs enabled in PM_MASK_SPI and if (doApply)
 - generate packet, update value if it changed


** apply_COUNTERS_UPD_RATE()
 - write 0 to 0x50 (counters read interval)
 - emptyCountBuffers();
 - readCountersDirectly();
 - write chosen value to 0x50 if in range - 0: no update; 1: 0.1s; 2: 0.2s, 3: 0.5s, 4: 1s; 5: 2s; 6: 5s; 7: 10s
If everything was successful, create log entry
*/

struct ConfigurationInfo {
    const SwtSequence seq;
    const optional<int16_t> delayA;
    const optional<int16_t> delayC;
};


class Configurations : public Iterativemapi {
    unordered_map<string, ConfigurationInfo> m_knownConfigurations;


    optional<int16_t> m_currDelayA;
    optional<int16_t> m_currDelayC;
    int16_t m_delayDifference;



    optional<SwtSequence> processDelayInput(optional<int16_t> delayA, optional<int16_t> delayC) {
        if(!delayA.has_value() && !delayC.has_value())
            return std::nullopt;

        string request;
        
        if(delayA.has_value()) {
            m_delayDifference = abs(delayA.value() - m_currDelayA.value_or(0));
            request += "DELAY_A,WRITE," + std::to_string(delayA.value()) + "\n";
            m_currDelayA = delayA;
        }

        if(delayC.has_value()) {
            int16_t cDelayDifference = abs(delayC.value() - m_currDelayC.value_or(0));
            if (cDelayDifference > m_delayDifference)
                m_delayDifference = cDelayDifference;
            request += "DELAY_C,WRITE," + std::to_string(delayC.value()) + "\n";
            m_currDelayC = delayC;
        }

        // return created request
    }


    string processInputMessage(string name) override {
        Utility::removeWhiteSpaces(name);
        if (m_knownConfigurations.count(name) == 0)
            throw std::runtime_error(name + ": no such configuration found");
        
        const ConfigurationInfo& cfg = m_knownConfigurations[name];

        optional<SwtSequence> delaySequence = processDelayInput(cfg.delayA, cfg.delayC);

        if(delaySequence.has_value()) {

        }     

    }

    string processOutputMessage(string msg) override {
        
    }
};