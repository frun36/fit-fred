#include "ResetFEE.h"
#include "TCM.h"
#include "PM.h"
#include "gbtInterfaceUtils.h"
#include <thread>

const BasicRequestHandler::ParsedResponse ResetFEE::EmptyResponse({ WinCCResponse(), {} });

void ResetFEE::processExecution()
{
    bool running = true;

    std::string request = waitForRequest(running);
    if (running == false) {
        return;
    }

    Print::PrintVerbose("Applying reset command");
    {
        auto response = applyResetFEE();
        if (response.errors.empty() == false) {
            publishError(response.getContents());
            return;
        }
    }
    Print::PrintVerbose("Constructing SPI mask");
    {
        auto response = checkPMLinks();
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
}

BasicRequestHandler::ParsedResponse ResetFEE::applyResetFEE()
{
    {
        auto parsedResponse = processSequence(*this, seqSwitchGBTErrorReports(false));
        if (parsedResponse.errors.empty() == false) {
            return parsedResponse;
        }
    }

    {
        auto parsedResponse = processSequence(*this, seqSetResetSystem());
        if (parsedResponse.errors.empty() == false) {
            return parsedResponse;
        }
    }

    std::this_thread::sleep_for(m_sleepAfterReset);

    {
        auto parsedResponse = processSequence(*this, seqSetResetFinished());
        if (parsedResponse.errors.empty() == false) {
            return parsedResponse;
        }
    }

    {
        auto parsedResponse = processSequence(*this, seqSwitchGBTErrorReports(true));
        if (parsedResponse.errors.empty() == false) {
            return parsedResponse;
        }
    }

    return EmptyResponse;
}

BasicRequestHandler::ParsedResponse ResetFEE::checkPMLinks()
{
    std::string pmRequest = pm_parameters::HighVoltage.data() + std::string(",READ");

    for (auto& pm: m_PMs) {
        uint32_t pmIdx = pm.getBoard()->getIdentity().number;
        {
            auto parsedResponse = processSequence(*this, seqMaskPMLink(pmIdx, true));
            if (parsedResponse.errors.empty() == false) {
                return parsedResponse;
            }
        }

        {
            auto parsedResponse = processSequence(pm, pmRequest);
            if (parsedResponse.errors.empty() == false) {
                (void)processSequence(*this, seqMaskPMLink(pmIdx, false));
            } else if (m_PMs[pmIdx].getBoard()->at(pm_parameters::HighVoltage.data()).getStoredValue() == 0xFFFFF) {
                (void)processSequence(*this, seqMaskPMLink(pmIdx, false));
            }
        }
    }

    return EmptyResponse;
}

BasicRequestHandler::ParsedResponse ResetFEE::applyGbtConfiguration()
{
    {
        auto parsedResponse = applyGbtConfigurationToBoard(*this);
        if (parsedResponse.errors.empty() == false) {
            return parsedResponse;
        }
    }

    auto checkSPIMask = [this](BasicRequestHandler& pmHandler) {
        return (static_cast<uint32_t>(this->m_board->at(tcm_parameters::PmSpiMask).getStoredValue()) &
                static_cast<uint32_t>(1u << pmHandler.getBoard()->getIdentity().number));
    };

    for (auto& pm : m_PMs)
    {
        if (!checkSPIMask(pm)) {
            continue;
        }
        auto parsedResponse = applyGbtConfigurationToBoard(pm);
        if (parsedResponse.errors.empty() == false) {
            return parsedResponse;
        }
    }

    return EmptyResponse;
}

BasicRequestHandler::ParsedResponse ResetFEE::applyGbtConfigurationToBoard(BasicRequestHandler& boardHandler)
{
    auto configuration = Configurations::BoardConfigurations::fetchConfiguration(gbt_config::GbtConfigurationName, boardHandler.getBoard()->getName());
    if (configuration.empty()) {
        return { WinCCResponse(), { { boardHandler.getBoard()->getName(), "Fatal! GBT configuration is not defined for this board!" } } };
    }
    std::string configReq = Configurations::BoardConfigurations::convertConfigToRequest(gbt_config::GbtConfigurationName, configuration);
    std::string request = configReq + "\n" + seqSetBoardId(boardHandler.getBoard()) + "\n" + seqSetSystemId();
    return processSequence(boardHandler, std::move(request));
}

std::string ResetFEE::seqSwitchGBTErrorReports(bool on)
{
    std::stringstream request;
    request << gbt_error::parameters::FifoReportReset << ",WRITE," << static_cast<int>(on);
    return request.str();
}

std::string ResetFEE::seqSetResetSystem()
{
    Board::ParameterInfo& forceLocalClock = m_board->at(tcm_parameters::ForceLocalClock.data());

    std::stringstream request;

    request << tcm_parameters::ResetSystem << ",WRITE,1\n";
    if (forceLocalClock.getStoredValueOptional() != std::nullopt && forceLocalClock.getStoredValue() == 1) {
        request << tcm_parameters::ForceLocalClock << ",WRITE,1";
    }

    return request.str();
}

std::string ResetFEE::seqSetResetFinished()
{
    std::stringstream request;
    request << tcm_parameters::SystemRestarted << ",WRITE," << 1;

    return request.str();
}

std::string ResetFEE::seqMaskPMLink(uint32_t idx, bool mask)
{
    Board::ParameterInfo& spiMask = m_board->at(tcm_parameters::PmSpiMask.data());
    if (spiMask.getStoredValueOptional() == std::nullopt) {
        spiMask.storeValue(0x0);
    }

    std::stringstream request;
    request << spiMask.name << ",WRITE,";
    request << (static_cast<uint32_t>(spiMask.getStoredValue()) | (static_cast<uint32_t>(mask) << idx));
    return request.str();
}

std::string ResetFEE::seqSetBoardId(std::shared_ptr<Board> board)
{
    Board::Identity identity = board->getIdentity();
    uint8_t id = 0;

    if (identity.type == Board::Type::PM && identity.side == Board::Side::A) {
        id = static_cast<uint8_t>(m_board->getEnvironment(environment::parameters::PmA0BoardId.data())) + identity.number;
    } else if (identity.type == Board::Type::PM && identity.side == Board::Side::C) {
        id = static_cast<uint8_t>(m_board->getEnvironment(environment::parameters::PmC0BoardId.data())) + identity.number;
    } else {
        id = static_cast<uint8_t>(m_board->getEnvironment(environment::parameters::TcmBoardId.data()));
    }
    std::stringstream request;
    request << gbt_config::parameters::BoardId << "WRITE," << id;
    return request.str();
}

std::string ResetFEE::seqSetSystemId()
{
    std::stringstream ss;
    ss << gbt_config::parameters::SystemId << ",WRITE," << m_board->getEnvironment(environment::parameters::SystemId.data());
    return ss.str();
}
