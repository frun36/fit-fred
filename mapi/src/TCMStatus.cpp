#include"TCMStatus.h"
#include<sstream>

TCMStatus::TCMStatus(std::shared_ptr<Board> board, std::list<std::string> toRefresh):
BasicRequestHandler(board)
{
    std::stringstream requestStream;
    for(auto& paramName: toRefresh)
    {
        requestStream << paramName << ",READ\n";
    }
    std::string request = requestStream.str();
    request.pop_back();
    m_request = processMessageFromWinCC(request);
}

std::string TCMStatus::processInputMessage(std::string req)
{
    return m_request.getSequence();
}

void TCMStatus::updateEnvironment()
{
    m_board->setEnvironment(SYSTEM_CLOCK_VNAME, (m_board->at(ACTUAL_SYSTEM_CLOCK_NAME).getStoredValue() == EXTERNAL_CLOCK) ?
                m_board->getEnvironment(EXTERNAL_CLOCK_VNAME):
                m_board->getEnvironment(INTERNAL_CLOCL_VNAME)
                );
    m_board->updateEnvironment(TDC_VNAME);
}

void TCMStatus::calculateGBTRate(WinCCResponse& response)
{
    double frequency = 1000.0/ static_cast<double>(m_pomTimeInterval.count());
    double wordsRate = (m_board->at(WORDS_COUNT_NAME).getStoredValue() - m_gbtRate.wordsCount) * frequency;
    double eventsRate =  (m_board->at(EVENTS_COUNT_NAME).getStoredValue() - m_gbtRate.eventsCount) * frequency;
    m_gbtRate.wordsCount = m_board->at(WORDS_COUNT_NAME).getStoredValue();
    m_gbtRate.eventsCount = m_board->at(EVENTS_COUNT_NAME).getStoredValue();
    response.addParameter(GBT_WORD_RATE_NAME, {wordsRate});
    response.addParameter(GBT_EVENT_RATE_NAME, {eventsRate});
}

std::string TCMStatus::processOutputMessage(std::string msg)
{
    m_currTimePoint = std::chrono::steady_clock::now();
    m_pomTimeInterval = std::chrono::duration_cast<std::chrono::milliseconds>(m_currTimePoint-m_lastTimePoint);
    
    auto parsedResponse = processMessageFromALF(msg);
    updateEnvironment();
    calculateGBTRate(parsedResponse.response);

    if(parsedResponse.errors.size() != 0)
    {
        returnError = true;
        std::stringstream error;
        for(auto& report: parsedResponse.errors)
        {
            error << report.what() << '\n';
        }
        error << parsedResponse.response.getContents();
        return error.str();
    }
    return parsedResponse.response.getContents();
}