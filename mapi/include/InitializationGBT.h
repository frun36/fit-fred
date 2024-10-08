#include"Fred/Mapi/indefinitemapi.h"
#include"BasicRequestHandler.h"

class InitializerGBT: public BasicRequestHandler
{
    public:
    struct Setting{
        uint32_t address;
        uint32_t value;
    };
    std::string getInitSequence();
};

