#pragma once
#include<cmath>
#include<cstdint>

template<typename IntegerType>
uint32_t twosComplementEncode(IntegerType value, uint32_t bitsNumber)
{
    //uint32_t mask = (bitsNumber == 32) ? 0xFFFFFFFFu : ((1u << bitsNumber) - 1u);
    return static_cast<uint32_t>(value) & ( (bitsNumber == 32u) ? 0xFFFFFFFFu : ((1u << bitsNumber) - 1u) );
}

template<typename IntegerType>
IntegerType twosComplementDecode(uint32_t code, uint32_t bitsNumber)
{
    if (bitsNumber == 32u)
    {
        return static_cast<IntegerType>(code);
    }
    if (code & (1u << (bitsNumber - 1u)))
    {
        uint32_t mask = ~((1u << bitsNumber) - 1u);
        return static_cast<IntegerType>(code | mask);
    }
    else
    {
        return static_cast<IntegerType>(code);
    }
}

constexpr uint32_t getBitField(uint32_t word, uint8_t first, uint8_t last)
{
    if((last - first + 1u) == 32u) return word;
    return static_cast<uint32_t>( (word >> first) & ( (1u << (last - first + 1u)) - 1u));
}

constexpr double maxUINT(uint8_t first, uint8_t last)
{
    if( (last - first + 1u) == 32u) return __UINT32_MAX__;
    return (1u << (last - first + 1u)) - 1u;
}