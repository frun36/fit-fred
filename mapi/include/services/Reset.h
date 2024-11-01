#include "BasicFitIndefiniteMapi.h"
#include "../BoardCommunicationHandler.h"
#include "Fred/Mapi/indefinitemapi.h"
#include "../utils.h"

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