#pragma once

#include <string>

#include "Fred/Mapi/mapi.h"
#include "board/BoardCommunicationHandler.h"

class Parameters : public Mapi
{
   public:
    Parameters(std::shared_ptr<Board> board) : m_boardHandler(board) {}
    string processInputMessage(string msg) override;
    string processOutputMessage(string msg) override;

   private:
    BoardCommunicationHandler m_boardHandler;
};
