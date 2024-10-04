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

BasicRequestHandler::ParsedResponse TCMStatus::processMessageFromALF(std::string alfresponse)
{
    WinCCResponse response;
	std::list<ErrorReport> report;

    try {
        AlfResponseParser alfMsg(alfresponse);
        if(!alfMsg.isSuccess()) {
            report.emplace_back("SEQUENCE", "ALF COMMUNICATION FAILED");
			return {std::move(response), std::move(report)};
        }

        for (auto line : alfMsg) {
            switch(line.type)
            {
                case AlfResponseParser::Line::Type::ResponseToRead:
                    unpackReadResponse(line, response, report);
                break;
                case AlfResponseParser::Line::Type::ResponseToWrite:
                break;
				default:
				throw std::runtime_error("Unsupported ALF response");
            }
            if(line)
        }

    } catch (const std::exception& e) {
		report.emplace_back("SEQUENCE", e.what());
        return {std::move(response), std::move(report)};
    }

	return {std::move(response), std::move(report)};
}

std::string TCMStatus::processOutputMessage(std::string msg)
{
    m_currTimePoint = std::chrono::steady_clock::now();
    m_pomTimeInterval = std::chrono::duration_cast<std::chrono::milliseconds>(m_currTimePoint-m_lastTimePoint);
    
    auto parsedResponse = processMessageFromALF(msg);
    
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