#include "services/templates/BasicFitIndefiniteMapi.h"
#include "board/BoardCommunicationHandler.h"

class Reset : public BasicFitIndefiniteMapi
{
   public:
    Reset(std::shared_ptr<Board> board);
    void processExecution() override;

   private:
    BoardCommunicationHandler m_boardHandler;
    std::unordered_map<std::string, Board::ParameterInfo&> m_resetParameters;
    std::string m_reqClearResetBits;
};
