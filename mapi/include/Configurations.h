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

class Configurations : public Mapigroup {
    unordered_map<string, unique_ptr<BoardConfiguration>> m_boardCofigurationServices;
    unordered_map<string, vector<string>> m_boardsInConfigurations;

    Configurations(Fred* fred, const unordered_map<string, Board>& boards) {
        for (const auto& boardPair : boards) {
            const string& boardName = boardPair.first;
            if (boardName == "TCM")
                m_boardCofigurationServices[boardName] = make_unique<TcmConfigurations>();
            else
                m_boardCofigurationServices[boardName] = make_unique<PmConfigurations>();
        }


        auto names = DatabaseInterface::executeQuery("SELECT UNIQUE configuration_name FROM configurations");

        for (const auto& wrappedConfigurationName : names) {
            const string& configurationName = wrappedConfigurationName[0]->getString();

            auto boardNames = DatabaseInterface::executeQuery("SELECT UNIQUE board_name FROM configurations WHERE configuration_name = " + configurationName);

            for (const auto& wrappedBoardName : boardNames) {
                const string& boardName = wrappedBoardName[0]->getString();
                m_boardsInConfigurations[configurationName].push_back(boardName);

                auto configurationInfo = 
                    DatabaseInterface::executeQuery("SELECT * FROM configurations WHERE configuration_name = " + configurationName + " && board_name = " + boardName);
                
                m_boardCofigurationServices[boardName]->registerConfiguration(configurationInfo);
            }
        }
    }
};

class BoardConfiguration : public BasicRequestHandler {
public:
    struct ConfigurationInfo {
        const SwtSequence seq;
        const optional<int16_t> delayA;
        const optional<int16_t> delayC;
    };

    virtual const unordered_map<string, ConfigurationInfo>& getKnownConfigurations() const = 0;
    virtual void registerConfiguration(vector<vector<MultiBase*>>) = 0;
};

class TcmConfigurations : public Iterativemapi, public BoardConfiguration {
    unordered_map<string, ConfigurationInfo> m_knownConfigurations;

    const unordered_map<string, ConfigurationInfo>& getKnownConfigurations() const override {
        return m_knownConfigurations;
    }

    void registerConfiguration(vector<vector<MultiBase*>>) override {
        //
    }

    optional<int16_t> m_currDelayA = nullopt;
    optional<int16_t> m_currDelayC = nullopt;
    int16_t m_delayDifference;
    const ConfigurationInfo* m_currCfg = nullptr;
    optional<ParsedResponse> m_delayResponse;

    enum class State { Idle, ApplyDelays, ApplyData } m_currState = State::Idle;

    static constexpr char* INTERNAL_PREFIX = "_INTERNAL:";

    optional<SwtSequence> processDelayInput(optional<int16_t> delayA, optional<int16_t> delayC) {
        if(!delayA.has_value() && !delayC.has_value())
            return nullopt;

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

        return processMessageFromWinCC(request);   
    }


    string processInputMessage(string msg) override {
        if (m_currState != State::Idle && strncmp(msg.c_str(), INTERNAL_PREFIX, strlen(INTERNAL_PREFIX)) != 0)
            throw std::runtime_error(msg + ": another configuration is already in progress");

        optional<SwtSequence> delaySequence;
        switch (m_currState) {
            case State::Idle:
                Utility::removeWhiteSpaces(msg);
                if (m_knownConfigurations.count(msg) == 0)
                    throw std::runtime_error(msg + ": no such configuration found");
                m_currCfg = &m_knownConfigurations[msg];
                delaySequence = processDelayInput(m_currCfg->delayA, m_currCfg->delayC);
                if(delaySequence.has_value()) {
                    m_currState = State::ApplyDelays;
                    return delaySequence->getSequence();
                }
                break;

            case State::ApplyDelays:
            case State::ApplyData:
                return m_currCfg->seq.getSequence();
        }
    }

    string processOutputMessage(string msg) override {
        auto response = processMessageFromALF(msg);

        switch (m_currState) {
            case State::ApplyDelays:

        }

        // parse alf response
        // generate response
        // if delays - send request for data
        // if data - ok
        // reset and counters?
    }
};

class PmConfigurations : public Mapi, public BoardConfiguration {
    const unordered_map<string, ConfigurationInfo>& m_knownConfigurations;

    const unordered_map<string, ConfigurationInfo>& getKnownConfigurations() const override {
        return m_knownConfigurations;
    }

    void registerConfiguration(vector<vector<MultiBase*>>) override {
        //
    }
    
    string processInputMessage(string msg) {
        Utility::removeWhiteSpaces(msg);
        if (m_knownConfigurations.count(msg) == 0)
            throw std::runtime_error(msg + ": no such configuration found");
        
        return m_knownConfigurations.at(msg).seq.getSequence();
    }

    string processOutputMessage(string msg) {
        auto parsedResponse = processMessageFromALF(msg);
        if(parsedResponse.errors.size() != 0) {
            returnError = true;
            std::stringstream error;
            for (auto& report: parsedResponse.errors)
                error << report.what() << '\n';
            error << parsedResponse.response.getContents();
            return error.str();
        }
        return parsedResponse.response.getContents();
    }
};