#include "services/ResetFEE.h"
#include "TCM.h"
#include "PM.h"
#include "gbtInterfaceUtils.h"
#include <thread>
#include<unistd.h>
void ResetFEE::processExecution()
{
    bool running = true;

    if(m_initialized == false){
        usleep(1e6); // wait for fred to start;
        auto response = updatePmSpiMask();
        if (response.errors.empty() == false) {
            printAndPublishError(response);
        }
        m_initialized = true;
    }

    std::string request = waitForRequest(running);
    if (running == false) {
        return;
    }

    if (request.find(ResetFEE::EnforceDefGbtConfig) != std::string::npos) {
        m_enforceDefGbtConfig = true;
    } else {
        m_enforceDefGbtConfig = false;
    }

    if (request.find(ResetFEE::ForceLocalClock) != std::string::npos) {
        m_forceLocalClock = true;
    } else {
        m_forceLocalClock = false;
    }

    Print::PrintVerbose("Applying reset command");
    {
        auto response = applyResetFEE();
        if (response.errors.empty() == false) {
            printAndPublishError(response);
            return;
        }
    }

    Print::PrintVerbose("Applying triggers signatures");
    {
        auto response = applyTriggersSign();
        if (response.errors.empty() == false) {
            printAndPublishError(response);
            return;
        }
    }

    Print::PrintVerbose("Constructing SPI mask");
    {
        auto response = updatePmSpiMask();
        if (response.errors.empty() == false) {
            printAndPublishError(response);
            return;
        }
    }
    Print::PrintVerbose("Applying GBT configuration");
    {
        auto response = applyGbtConfiguration();
        if (response.errors.empty() == false) {
            printAndPublishError(response);
            return;
        }
    }
    publishAnswer("SUCCESS");
}

BoardCommunicationHandler::ParsedResponse ResetFEE::applyResetFEE()
{
    uint8_t cntUpdateRate = static_cast<uint8_t>(m_TCM.getBoard()->at(tcm_parameters::CounterReadInterval).getElectronicValueOptional().value_or(0));

    {
        auto parsedResponse = processSequenceThroughHandler(m_TCM, seqSwitchGBTErrorReports(false));
        if (parsedResponse.errors.empty() == false) {
            return parsedResponse;
        }
    }

    {
        auto parsedResponse = processSequenceThroughHandler(m_TCM, seqSetResetSystem(), false);
        if (parsedResponse.errors.empty() == false) {
            return parsedResponse;
        }
    }

    std::this_thread::sleep_for(m_sleepAfterReset);

    {
        auto parsedResponse = processSequenceThroughHandler(m_TCM, seqSetResetFinished(), false);
        if (parsedResponse.errors.empty() == false) {
            return parsedResponse;
        }
    }

    {
        auto parsedResponse = processSequenceThroughHandler(m_TCM, seqSwitchGBTErrorReports(true));
        if (parsedResponse.errors.empty() == false) {
            return parsedResponse;
        }
    }

    {
        auto parsedResponse = processSequenceThroughHandler(m_TCM, seqCntUpdateRate(cntUpdateRate));
        if (parsedResponse.errors.empty() == false) {
            return parsedResponse;
        }
    }

    return BoardCommunicationHandler::ParsedResponse::EmptyResponse;
}

BoardCommunicationHandler::ParsedResponse ResetFEE::updatePmSpiMask()
{
    bool isConnected[20] = {false};
    std::string pmRequest = WinCCRequest::readRequest(pm_parameters::SupplyVoltage1_8V);
    Board::ParameterInfo& spiMask = m_TCM.getBoard()->at(tcm_parameters::PmSpiMask.data());
    
    for (auto& pm : m_PMs) {
        uint32_t pmIdx = pm.getBoard()->getIdentity().number;
        uint32_t baseIdx = (pm.getBoard()->getIdentity().side == Board::Side::C) ? 10 : 0;
        isConnected[pmIdx+baseIdx] = true;
        {
            auto parsedResponse = processSequenceThroughHandler(m_TCM, seqMaskPMLink(pmIdx + baseIdx, true));
            if (parsedResponse.errors.empty() == false) {
                return parsedResponse;
            }
        }
        {
            auto parsedResponse = processSequenceThroughHandler(pm, pmRequest);
            if (parsedResponse.errors.empty() == false) {
                (void)processSequenceThroughHandler(m_TCM, seqMaskPMLink(pmIdx + baseIdx, false));
            } else if (pm.getBoard()->at(pm_parameters::SupplyVoltage1_8V.data()).getElectronicValue() == 0xFFFFFFFF) {
                (void)processSequenceThroughHandler(m_TCM, seqMaskPMLink(pmIdx + baseIdx, false));
            }
        }
    }

    uint32_t currentMask = spiMask.getElectronicValue();
    for(int idx = 0; idx < 20; idx++){
        if(isConnected[idx] == false){
            currentMask = currentMask & (~(static_cast<uint32_t>(1u) << idx));
             Print::PrintData(string_utils::concatenate("PM",(idx >= 10 ? "C" : "A"), std::to_string(idx >= 10 ? idx - 10: idx)," is not connected"));
        } else if((currentMask >> idx) == 0){
            Print::PrintData(string_utils::concatenate("PM",(idx >= 10 ? "C" : "A"), std::to_string(idx >= 10 ? idx - 10: idx)," is not connected"));
        } else {
            Print::PrintData(string_utils::concatenate("PM",(idx >= 10 ? "C" : "A"), std::to_string(idx >= 10 ? idx - 10: idx)," is connected"));
        }
    }
    {
        auto parsedResponse = processSequenceThroughHandler(m_TCM, WinCCRequest::writeRequest(tcm_parameters::PmSpiMask, currentMask));
        if (parsedResponse.errors.empty() == false) {
            return parsedResponse;
        }
    }

    return BoardCommunicationHandler::ParsedResponse::EmptyResponse;
}

BoardCommunicationHandler::ParsedResponse ResetFEE::applyGbtConfiguration()
{
    auto isBoardIdIncorrect = [this](std::shared_ptr<Board> board) {
        return ((static_cast<uint32_t>(board->at(gbt::parameters::BoardId.data()).getPhysicalValue()) != this->getEnvBoardId(board)) ||
                (board->at(gbt::parameters::SystemId).getPhysicalValue() != m_TCM.getBoard()->getEnvironment(environment::parameters::SystemId.data())));
    };
    std::string readFEEId = WinCCRequest::readRequest(gbt::parameters::BoardId) + "\n" + WinCCRequest::readRequest(gbt::parameters::SystemId);

    // Reading TCM ID
    {
        auto parsedResponse = processSequenceThroughHandler(m_TCM, readFEEId);
        if (parsedResponse.errors.empty() == false) {
            return parsedResponse;
        }
    }

    if (isBoardIdIncorrect(m_TCM.getBoard()) || m_enforceDefGbtConfig) {
        auto parsedResponse = applyGbtConfigurationToBoard(m_TCM);
        if (parsedResponse.errors.empty() == false) {
            return parsedResponse;
        }
    }

    auto checkSPIMask = [this](BoardCommunicationHandler& pmHandler) {
        return (static_cast<uint32_t>(this->m_TCM.getBoard()->at(tcm_parameters::PmSpiMask).getPhysicalValue()) &
                static_cast<uint32_t>(1u << pmHandler.getBoard()->getIdentity().number));
    };

    for (auto& pm : m_PMs) {
        if (!checkSPIMask(pm)) {
            continue;
        }
        // Reading PM ID
        {
            auto parsedResponse = processSequenceThroughHandler(pm, readFEEId);
            if (parsedResponse.errors.empty() == false) {
                return parsedResponse;
            }
        }
        // Comparing ID readed from board to ID calculated from the environment variables
        if (isBoardIdIncorrect(pm.getBoard()) || m_enforceDefGbtConfig) {
            auto parsedResponse = applyGbtConfigurationToBoard(pm);
            if (parsedResponse.errors.empty() == false) {
                return parsedResponse;
            }
        }
    }

    return BoardCommunicationHandler::ParsedResponse::EmptyResponse;
}

BoardCommunicationHandler::ParsedResponse ResetFEE::applyGbtConfigurationToBoard(BoardCommunicationHandler& boardHandler)
{
    auto configuration = BoardConfigurations::fetchConfiguration(gbt::GbtConfigurationName, boardHandler.getBoard()->getName());
    if (configuration.empty()) {
        return { WinCCResponse(), { { boardHandler.getBoard()->getName(), "Fatal! GBT configuration is not defined!" } } };
    }

    std::stringstream request;

    request << BoardConfigurations::parseConfigurationInfo(gbt::GbtConfigurationName, configuration).req;
    if (boardHandler.getBoard()->at(gbt::parameters::BcIdDelay).getPhysicalValueOptional() == std::nullopt) {
        request << WinCCRequest::writeRequest(gbt::parameters::BcIdDelay,
                                              static_cast<uint32_t>(m_TCM.getBoard()->getEnvironment(environment::parameters::BcIdOffsetDefault.data())))
                << "\n";
    } else {
        request << WinCCRequest::writeRequest(gbt::parameters::BcIdDelay, boardHandler.getBoard()->at(gbt::parameters::BcIdDelay).getPhysicalValue()) << "\n";
    }

    request << seqSetBoardId(boardHandler.getBoard()) << "\n";
    request << seqSetSystemId();

    return processSequenceThroughHandler(boardHandler, request.str());
}

std::string ResetFEE::seqCntUpdateRate(uint8_t updateRate)
{
    return WinCCRequest::writeRequest(tcm_parameters::CounterReadInterval, updateRate);
}   

std::string ResetFEE::seqSwitchGBTErrorReports(bool on)
{
    return WinCCRequest::writeRequest(gbt::parameters::FifoReportReset, static_cast<int>(!on));
}

std::string ResetFEE::seqSetResetSystem()
{
    std::stringstream request;
    request << WinCCRequest::writeRequest(tcm_parameters::ResetSystem, 1) << "\n";
    request << WinCCRequest::writeRequest(tcm_parameters::ForceLocalClock, m_forceLocalClock);
    return request.str();
}

std::string ResetFEE::seqSetResetFinished()
{
    return WinCCRequest::writeRequest(tcm_parameters::SystemRestarted, 1);
}

std::string ResetFEE::seqMaskPMLink(uint32_t idx, bool mask)
{
    Board::ParameterInfo& spiMask = m_TCM.getBoard()->at(tcm_parameters::PmSpiMask.data());
    uint32_t masked = static_cast<uint32_t>(spiMask.getPhysicalValue()) & (~(static_cast<uint32_t>(1u) << idx));
    masked |= static_cast<uint32_t>(mask) << idx;

    return WinCCRequest::writeRequest(spiMask.name, masked);
}

std::string ResetFEE::seqSetBoardId(std::shared_ptr<Board> board)
{
    return WinCCRequest::writeRequest(gbt::parameters::BoardId, getEnvBoardId(board));
}

uint32_t ResetFEE::getEnvBoardId(std::shared_ptr<Board> board)
{
    Board::Identity identity = board->getIdentity();
    uint32_t id = 0;

    if (identity.type == Board::Type::PM && identity.side == Board::Side::A) {
        id = static_cast<uint32_t>(m_TCM.getBoard()->getEnvironment(environment::parameters::PmA0BoardId.data())) + identity.number;
    } else if (identity.type == Board::Type::PM && identity.side == Board::Side::C) {
        id = static_cast<uint32_t>(m_TCM.getBoard()->getEnvironment(environment::parameters::PmC0BoardId.data())) + identity.number;
    } else {
        id = static_cast<uint32_t>(m_TCM.getBoard()->getEnvironment(environment::parameters::TcmBoardId.data()));
    }
    return id;
}

std::string ResetFEE::seqSetSystemId()
{
    return WinCCRequest::writeRequest(gbt::parameters::SystemId, m_TCM.getBoard()->getEnvironment(environment::parameters::SystemId.data()));
}

BoardCommunicationHandler::ParsedResponse ResetFEE::applyTriggersSign()
{
    std::stringstream request;
    double trigger1Sign = static_cast<double>(m_TCM.getBoard()->getEnvironment(environment::parameters::Trigger1Signature.data()));
    double trigger2Sign = static_cast<double>(m_TCM.getBoard()->getEnvironment(environment::parameters::Trigger2Signature.data()));
    double trigger3Sign = static_cast<double>(m_TCM.getBoard()->getEnvironment(environment::parameters::Trigger3Signature.data()));
    double trigger4Sign = static_cast<double>(m_TCM.getBoard()->getEnvironment(environment::parameters::Trigger4Signature.data()));
    double trigger5Sign = static_cast<double>(m_TCM.getBoard()->getEnvironment(environment::parameters::Trigger5Signature.data()));

    request << WinCCRequest::writeRequest(tcm_parameters::Trigger1Signature, trigger1Sign) << "\n";
    request << WinCCRequest::writeRequest(tcm_parameters::Trigger2Signature, trigger2Sign) << "\n";
    request << WinCCRequest::writeRequest(tcm_parameters::Trigger3Signature, trigger3Sign) << "\n";
    request << WinCCRequest::writeRequest(tcm_parameters::Trigger4Signature, trigger4Sign) << "\n";
    request << WinCCRequest::writeRequest(tcm_parameters::Trigger5Signature, trigger5Sign);
    return processSequenceThroughHandler(m_TCM, request.str());
}