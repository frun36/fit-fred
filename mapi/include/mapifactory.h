#include "Fred/fred.h"
#include "Fred/Mapi/mapi.h"
#include "Alfred/print.h"
#include"Parameters.h"
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>

class MapiFactory {
private:
    Fred* fred;

    vector<Mapi*> mapiObjects;
    std::vector<Parameters> parametersObject;
    
    void generateObjects();
public:
    MapiFactory(Fred* fred);
    virtual ~MapiFactory();
};
