#include "BasicFitIndefiniteMapi.h"

class SetPhaseDelay : public BasicFitIndefiniteMapi
{
   public:
    SetPhaseDelay(std::shared_ptr<Board> board) : m_handler(board)
    {
    }
    void processExecution() override;

   private:
    BoardCommunicationHandler m_handler;
};