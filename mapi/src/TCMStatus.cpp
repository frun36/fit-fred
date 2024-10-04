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


std::string TCMStatus::processOutputMessage(std::string msg)
{
    m_currTimePoint = std::chrono::steady_clock::now();
    m_pomTimeInterval = std::chrono::duration_cast<std::chrono::milliseconds>(m_currTimePoint-m_lastTimePoint);
    
    auto parsedResponse = processMessageFromALF(msg);
    
    m_board->setEnvironment(SYSTEM_CLOCK_VNAME, (m_board->at(ACTUAL_SYSTEM_CLOCK_NAME).getStoredValue() == EXTERNAL_CLOCK) ?
                m_board->getEnvironment(EXTERNAL_CLOCK_VNAME):
                m_board->getEnvironment(INTERNAL_CLOCL_VNAME)
                );
    m_board->updateEnvironment(TDC_VNAME);

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