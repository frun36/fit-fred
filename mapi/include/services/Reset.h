#include "../BasicRequestHandler.h"
#include "Fred/Mapi/indefinitemapi.h"
#include"../utils.h"

class Reset: public BasicRequestHandler, public IndefiniteMapi
{
public:
    Reset(std::shared_ptr<Board> board);
    void processExecution() override;
private:
    BasicRequestHandler::ParsedResponse processSequence(std::string request)
    {
        std::string seq;
        try {
            seq = processMessageFromWinCC(request).getSequence();
        } catch (const std::exception& e) {
            return { WinCCResponse(), { { m_board->getName(), e.what() } } };
        }
        return processMessageFromALF(executeAlfSequence(seq));
    }

    template <typename T>
    std::string writeRequest(std::string_view param, T value)
    {
        return string_utils::concatenate(param, ",WRITE,", std::to_string(value));
    }

    std::string readRequest(std::string_view param)
    {
        return string_utils::concatenate(param, ",READ");
    }

    std::string& appendRequest(std::string& mess, const std::string& newRequest)
    {
        return mess.append(newRequest).append("\n");
    }

    std::unordered_map<std::string, Board::ParameterInfo&> m_resetParameters;
    std::string m_reqClearResetBits;
};