#include "services/ResetFEE.h"
#include "TCM.h"
#include "PM.h"
#include "gbtInterfaceUtils.h"
#include <thread>

void ResetFEE::processExecution()
{
    bool running = true;

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
            publishError(response.getContents());
            return;
        }
    }

    Print::PrintVerbose("Applying triggers signatures");
    {
        auto response = applyTriggersSign();
        if (response.errors.empty() == false) {
            publishError(response.getContents());
            return;
        }
    }

    Print::PrintVerbose("Constructing SPI mask");
    {
        auto response = testPMLinks();
        if (response.errors.empty() == false) {
            publishError(response.getContents());
            return;
        }
    }
    Print::PrintVerbose("Applying GBT configuration");
    {
        auto response = applyGbtConfiguration();
        if (response.errors.empty() == false) {
            publishError(response.getContents());
            return;
        }
    }
    publishAnswer("SUCCESS");
}

BoardCommunicationHandler::ParsedResponse ResetFEE::applyResetFEE()
{
    m_TCM.getBoard()->lock();
    for(auto& PM : m_PMs){
        PM.getBoard()->lock();
    }

    {
        auto parsedResponse = processSequenceThroughHandler(m_TCM, seqSwitchGBTErrorReports(false));
        if (parsedResponse.errors.empty() == false) {
            return parsedResponse;
        }
    }

    {
        auto parsedResponse = processSequenceThroughHandler(m_TCM, seqSetResetSystem());
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

    m_TCM.getBoard()->unlock();
    for(auto& PM : m_PMs){
        PM.getBoard()->unlock();
    }

    return EmptyResponse;
}

BoardCommunicationHandler::ParsedResponse ResetFEE::testPMLinks()
{
    std::string pmRequest = WinCCRequest::readRequest(pm_parameters::HighVoltage);

    for (auto& pm : m_PMs) {
        uint32_t pmIdx = pm.getBoard()->getIdentity().number;
        {
            auto parsedResponse = processSequenceThroughHandler(m_TCM, seqMaskPMLink(pmIdx, true));
            if (parsedResponse.errors.empty() == false) {
                return parsedResponse;
            }
        }

        {
            auto parsedResponse = processSequenceThroughHandler(pm, pmRequest);
            if (parsedResponse.errors.empty() == false) {
                (void)processSequenceThroughHandler(m_TCM, seqMaskPMLink(pmIdx, false));
            } else if (m_PMs[pmIdx].getBoard()->at(pm_parameters::HighVoltage.data()).getPhysicalValue() == 0xFFFFF) {
                (void)processSequenceThroughHandler(m_TCM, seqMaskPMLink(pmIdx, false));
            }
        }
    }

    return EmptyResponse;
}

BoardCommunicationHandler::ParsedResponse ResetFEE::applyGbtConfiguration()
{
    auto isBoardIdCorrect = [this](std::shared_ptr<Board> board) {
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

    if (isBoardIdCorrect(m_TCM.getBoard()) || m_enforceDefGbtConfig) {
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
        if (isBoardIdCorrect(pm.getBoard()) || m_enforceDefGbtConfig) {
            auto parsedResponse = applyGbtConfigurationToBoard(pm);
            if (parsedResponse.errors.empty() == false) {
                return parsedResponse;
            }
        }
    }

    return EmptyResponse;
}

BoardCommunicationHandler::ParsedResponse ResetFEE::applyGbtConfigurationToBoard(BoardCommunicationHandler& boardHandler)
{
    auto configuration = Configurations::BoardConfigurations::fetchConfiguration(gbt::GbtConfigurationName, gbt::GbtConfigurationBoardName);
    if (configuration.empty()) {
        return { WinCCResponse(), { { boardHandler.getBoard()->getName(), "Fatal! GBT configuration is not defined!" } } };
    }

    std::stringstream request;

    request << Configurations::BoardConfigurations::convertConfigToRequest(gbt::GbtConfigurationName, configuration);
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

std::string ResetFEE::seqSwitchGBTErrorReports(bool on)
{
    return WinCCRequest::writeRequest(gbt::parameters::FifoReportReset, static_cast<int>(!on));
}

std::string ResetFEE::seqSetResetSystem()
{
    std::stringstream request;

    request << WinCCRequest::writeRequest(tcm_parameters::ResetSystem, 1) << "\n";
    if (m_forceLocalClock) {
        request << WinCCRequest::writeRequest(tcm_parameters::ForceLocalClock, 1);
    }

    return request.str();
}

std::string ResetFEE::seqSetResetFinished()
{
    return WinCCRequest::writeRequest(tcm_parameters::SystemRestarted, 1);
}

std::string ResetFEE::seqMaskPMLink(uint32_t idx, bool mask)
{
    Board::ParameterInfo& spiMask = m_TCM.getBoard()->at(tcm_parameters::PmSpiMask.data());
    if (spiMask.getPhysicalValueOptional() == std::nullopt) {
        spiMask.storeValue(0x0,0x0);
    }

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