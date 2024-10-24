#pragma once

#include <unordered_map>
#include <string>

#include "Fred/Mapi/mapi.h"
#include "BasicRequestHandler.h"

class Parameters : public Mapi, BasicRequestHandler
{
   public:
    Parameters(std::shared_ptr<Board> board) : BasicRequestHandler(board) {}
    string processInputMessage(string msg) override;
    string processOutputMessage(string msg) override;
};