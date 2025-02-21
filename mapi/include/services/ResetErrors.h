#include "services/templates/BasicFitIndefiniteMapi.h"
#include "board/BoardCommunicationHandler.h"

class ResetErrors : public BasicFitIndefiniteMapi
{
   public:
    ResetErrors(std::shared_ptr<Board> tcm, std::vector<std::shared_ptr<Board>> pms);
    void processExecution() override;

   private:
    BoardCommunicationHandler::ParsedResponse applyResetBoard(BoardCommunicationHandler& boardHandler);
    std::string getReqClearErrors();

    std::vector<BoardCommunicationHandler> m_PMs;
    BoardCommunicationHandler m_TCM;
    std::string m_reqClearResetBits;
    std::string m_reqClearAndUnlock;
    std::string m_reqApplyResets;
};
