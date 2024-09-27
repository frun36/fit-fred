#pragma once

#include<list>
#include<unordered_map>
#include<chrono>
#include "Fred/Mapi/mapi.h"
#include"BasicRequestHandler.h"

class BoardRefresh: public Mapi, BasicRequestHandler{
public:
    BoardRefresh(std::shared_ptr<Board> board, std::list<std::string> toRefresh);
    std::string processInputMessage(string msg) override;
    std::string processOutputMessage(string msg) override;
private:
    SwtSequence m_request;
};