#pragma once

#include "services/configurations/BoardConfigurations.h"
#include <unordered_map>
#include <cstring>
#include <memory>

#ifdef FIT_UNIT_TEST

#include "../../test/mocks/include/databaseinterface.h"
#include "../../test/mocks/include/mapi.h"
#include "../../test/mocks/include/utility.h"
#include "gtest/gtest.h"

#else

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
    Configurations() = default;
    Configurations(const string& fredName, const unordered_map<string, shared_ptr<Board>>& boards);

    vector<string> fetchBoardNamesToConfigure(const string& configurationName) const;

    string processInputMessage(string msg) override;
    string processOutputMessage(string msg) override;

    const unordered_map<string, unique_ptr<BoardConfigurations>>& getBoardConfigurationServices() const { return m_boardCofigurationServices; }

   private:
    unordered_map<string, unique_ptr<BoardConfigurations>> m_boardCofigurationServices;
    string m_fredName;
};
