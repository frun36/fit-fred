#include"ResetFEE.h"
#include"TCMParameters.h"
#include"PMParameters.h"
#include"PMParameters.h"
#include<thread>
void ResetFEE::processExecution()
{
    bool running = true;

    std::string request = waitForRequest(running);
    if(running == false) {
            return;
    }
        
    {
        auto response = applyResetFEE();
        if(response.errors.empty() == false){
            publishError(parseErrorString(response));
        }
    }
    {
        auto response = checkPMLinks();
        if(response.errors.empty() == false){
            publishError(parseErrorString(response));
        }
    }


}


BasicRequestHandler::ParsedResponse ResetFEE::applyResetFEE()
{
    auto processSequence = 
    [this](SwtSequence&& sequence)
    {
        return this->processMessageFromALF(this->executeAlfSequence(sequence.getSequence()));
    };

    resetExecutionData();

    {
        auto parsedResponse = processSequence(switchGBTErrorReports(false));
        if(parsedResponse.errors.empty() == false)
        {
            return parsedResponse;
        }
    }

    {
        auto parsedResponse = processSequence(setRestSystem());
        if(parsedResponse.errors.empty() == false)
        {
            return parsedResponse;
        }
    }

    std::this_thread::sleep_for(m_sleepAfterReset);

    return processSequence(setResetFinished());
}

SwtSequence ResetFEE::switchGBTErrorReports(bool on)
{
    std::string request{tcm_parameters::GBT_RESET_ERROR_REPORT_FIFO};
    request.append("WRITE\n").append(std::to_string(!on));
    return appendMessageFromWinCC(request);
    return appendMessageFromWinCC(request);
}

SwtSequence ResetFEE::setRestSystem()
{
    Board::ParameterInfo& forceLocalClock = m_board->at(tcm_parameters::BOARD_STATUS_FORCE_LOCAL_CLOCK.data());

    std::stringstream request;
    std::vector<std::string_view> boardStatusParameters = tcm_parameters::getBoardStatusParameters();
   
    for(auto& parameter: boardStatusParameters){
        if(parameter == tcm_parameters::BOARD_STATUS_RESET_SYSTEM){
            request << parameter << ",WRITE,1\n";
        }
        else if(parameter == tcm_parameters::BOARD_STATUS_FORCE_LOCAL_CLOCK && forceLocalClock.getStoredValueOptional() != std::nullopt && forceLocalClock.getStoredValue() == 1){
            request << parameter << ",WRITE,1\n";
        }
        request << parameter << ",WRITE,0\n";
    }
    processMessageFromWinCC(request.str());
    return switchGBTErrorReports(true);
}

SwtSequence ResetFEE::setResetFinished()
{
    std::stringstream request;
    std::vector<std::string_view> boardStatusParameters = tcm_parameters::getBoardStatusParameters();
    for(auto& parameter: boardStatusParameters)
    {
        if(parameter == tcm_parameters::BOARD_STATUS_SYSTEM_RESTARTED)
        {
            request << parameter << ",WRITE,1\n";
        }
        request << parameter << ",WRITE,0\n";
    }
    request << tcm_parameters::GBT_RESET_ERROR_REPORT_FIFO << ",WRITE,0\n";
    return processMessageFromWinCC(request.str());
}

SwtSequence ResetFEE::maskPMLink(uint32_t idx, bool mask)
{
    Board::ParameterInfo& spiMask = m_board->at(tcm_parameters::PM_SPI_MASK.data());
    if(spiMask.getStoredValueOptional() == std::nullopt)
    {
        spiMask.storeValue(0x0);
    }

    std::stringstream request;
    request << tcm_parameters::PM_SPI_MASK <<",WRITE,";
    request << std::hex << (static_cast<uint32_t>(spiMask.getStoredValue()) | (static_cast<uint32_t>(mask)<<idx));
    return processMessageFromWinCC(request.str());
}

BasicRequestHandler::ParsedResponse ResetFEE::checkPMLinks()
{
    auto processSequencePM = [this](BasicRequestHandler& handler, const std::string& request)
    {
        auto sequence = handler.processMessageFromWinCC(request);
        return handler.processMessageFromALF(this->executeAlfSequence(sequence.getSequence()));
    };

    auto processSequenceTCM = [this](SwtSequence&& sequence)
    {
        return this->processMessageFromALF(this->executeAlfSequence(sequence.getSequence()));
    };

    std::string pmRequest = pm_parameters::HIGH_VOLTAGE.data() + std::string(",READ");

    for(uint32_t pmIdx = 0; pmIdx < 10; pmIdx++)
    {
        {
            auto parsedResponse = processSequenceTCM(maskPMLink(pmIdx, true));
            if(parsedResponse.errors.empty() == false){
                return parsedResponse;
            }
        }

        {
            auto parsedResponse = processSequencePM(m_PMs[pmIdx], pmRequest);
            if(parsedResponse.errors.empty() == false){
                (void) processSequenceTCM(maskPMLink(pmIdx, false));
            }
            else if(m_PMs[pmIdx].getBoard()->at(pm_parameters::HIGH_VOLTAGE.data()).getStoredValue() == 0xFFFFF){
                (void) processSequenceTCM(maskPMLink(pmIdx, false));
            }
        }
    }
}


