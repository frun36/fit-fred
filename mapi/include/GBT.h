#include<cstdint>

struct GBTWord
{
    static constexpr uint32_t WordSize = 5;
    uint16_t buffer[WordSize];
};


union GBTErrorReport
{
    enum ErrorCode{BCSyncLostInRun = 0xEEEE000A, PMEarlyHeader = 0xEEEE0009, FifoOverload = 0xEEEE0008};
    static constexpr uint32_t Size = 36;

    struct {
    
    uint32_t errorCode;
    union {
        struct TypeBCsyncLostInRun {
                struct {
                    GBTWord data;
                    uint16_t counter : 12,
                            isData  :  4;
                } words[10];                    
                uint16_t BC_CRU; 
                uint16_t BC_Board;   
                uint32_t orbitBoard;         
                uint32_t orbitCRU;         
                uint32_t reservedSpace[2];  
            } SL;

            struct TypePMearlyHeader {
                GBTWord w[14];
            } EH;
            
            struct TypeFIFOoverload {
                GBTWord w[13];              
                uint16_t reservedSpace0;    
                uint16_t rdRate;
                uint16_t wrRate;     
                uint32_t _reservedSpace1;    
            } FO;
    };

    } data;

    uint32_t buffer[Size];
};
