#include "BasicFitIndefiniteMapi.h"
#include "../BasicRequestHandler.h"
#include "Fred/Mapi/indefinitemapi.h"
#include "../utils.h"

class ResetErrors : public BasicFitIndefiniteMapi
{
   public:
    ResetErrors(std::shared_ptr<Board> tcm, std::vector<std::shared_ptr<Board>> pms);
    void processExecution() override;

   private:
    BasicRequestHandler::ParsedResponse applyResetBoard(BasicRequestHandler& boardHandler);
    std::string getReqClearErrors();

    std::vector<BasicRequestHandler> m_PMs;
    std::string m_reqClearResetBits;
    std::string m_reqClearAndUnlock;
    std::string m_reqApplyResets;
};