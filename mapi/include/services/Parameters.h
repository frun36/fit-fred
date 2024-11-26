#pragma once

#include <unordered_map>
#include <string>

#include "Fred/Mapi/mapi.h"
#include "BoardCommunicationHandler.h"

class Parameters : public Mapi
{
   public:
    Parameters(std::shared_ptr<Board> board) : m_boardHandler(board) {}
    string processInputMessage(string msg) override;
    string processOutputMessage(string msg) override;

   private:
    BoardCommunicationHandler m_boardHandler;
};