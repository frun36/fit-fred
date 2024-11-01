#pragma once

#include <list>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <sstream>
#include "communication-utils/SwtSequence.h"
#include "communication-utils/AlfResponseParser.h"
#include "communication-utils/WinCCRequest.h"
#include "communication-utils/WinCCResponse.h"
#include "Board.h"

class BoardCommunicationHandler
{
   public:
    BoardCommunicationHandler(std::shared_ptr<Board> board) : m_board(board)
    {
    }
    virtual ~BoardCommunicationHandler() = default;

    struct ErrorReport {
        ErrorReport() = default;
        ErrorReport(const std::string& param, const std::string& message) : parameterName(param), mess(message)
        {
        }

        const std::string parameterName;
        const std::string mess;

        std::string what() const
        {
            return "ERROR - " + parameterName + ": " + mess;
        }
    };

    struct ParsedResponse {
        ParsedResponse(WinCCResponse&& response_, std::list<ErrorReport>&& errors_) : response(response_), errors(errors_) {}
        const WinCCResponse response;
        const std::list<ErrorReport> errors;

        bool isError() const { return errors.size() != 0; }

        string getContents() const
        {
            if (!isError())
                return response.getContents();

            std::stringstream ss;
            for (auto& report : errors)
                ss << report.what() << '\n';
            ss << response.getContents();
            return ss.str();
        }
    };

    SwtSequence processMessageFromWinCC(std::string, bool = true);
    virtual ParsedResponse processMessageFromALF(std::string);

    std::shared_ptr<Board> getBoard() { return m_board; }

   protected:
    struct ParameterToHandle {
        ParameterToHandle(const std::string& name_, const std::optional<double>& toCompare_) : name(name_), toCompare(toCompare_) {}
        ParameterToHandle(const std::string& name_, std::optional<double>&& toCompare_) : name(name_), toCompare(toCompare_) {}
        std::string name;
        std::optional<uint32_t> toCompare;
    };

    void resetExecutionData();
    void mergeOperation(SwtSequence::SwtOperation& operation, SwtSequence::SwtOperation& toMerge);
    SwtSequence::SwtOperation createSwtOperation(const WinCCRequest::Command& command) const;

    void unpackReadResponse(const AlfResponseParser::Line& read, WinCCResponse& response, std::list<ErrorReport>& report);

    typedef std::list<ParameterToHandle> ToHandleList;

    std::unordered_map<uint32_t, ToHandleList> m_registerTasks;
    std::unordered_map<uint32_t, SwtSequence::SwtOperation> m_operations;

    std::shared_ptr<Board> m_board;
};
