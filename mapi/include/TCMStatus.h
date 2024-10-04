#pragma once

#include<list>
#include<unordered_map>
#include<chrono>
#include "Fred/Mapi/mapi.h"
#include"BasicRequestHandler.h"

class TCMStatus: public Mapi, BasicRequestHandler{
public:
    TCMStatus(std::shared_ptr<Board> board, std::list<std::string> toRefresh);
    std::string processInputMessage(string msg) override;
    std::string processOutputMessage(string msg) override;
    BasicRequestHandler::ParsedResponse processMessageFromALF(std::string alfresponse) override;
private:
    SwtSequence m_request;

    std::chrono::milliseconds m_pomTimeInterval;
    std::chrono::time_point<std::chrono::steady_clock> m_lastTimePoint;
    std::chrono::time_point<std::chrono::steady_clock> m_currTimePoint;
};