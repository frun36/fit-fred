#include "BasicFitIndefiniteMapi.h"
#include "../BasicRequestHandler.h"
#include "Fred/Mapi/indefinitemapi.h"
#include "../utils.h"

class Reset : public BasicFitIndefiniteMapi
{
   public:
    Reset(std::shared_ptr<Board> board);
    void processExecution() override;

   private:
    std::unordered_map<std::string, Board::ParameterInfo&> m_resetParameters;
    std::string m_reqClearResetBits;
};