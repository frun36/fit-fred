#ifndef UTILS_FIT
#define UTILS_FIT

#include <cmath>
#include <cstdint>

template <typename IntegerType, typename RawType>
RawType twosComplementEncode(IntegerType value, uint32_t bitsNumber)
{
    if (bitsNumber == sizeof(RawType) * 8u) {
        return static_cast<RawType>(value);
    } else {
        RawType mask = (static_cast<RawType>(1u) << bitsNumber) - 1u;
        return static_cast<RawType>(value) & mask;
    }
}

template <typename IntegerType>
uint32_t twosComplementEncode(IntegerType value, uint32_t bitsNumber)
{
    return twosComplementEncode<IntegerType, uint32_t>(value, bitsNumber);
}

template <typename IntegerType, typename RawType>
IntegerType twosComplementDecode(RawType code, uint32_t bitsNumber)
{
    static_assert(sizeof(IntegerType) == sizeof(RawType),
                  "IntegerType and RawType must be the same size");

    if (bitsNumber == sizeof(RawType) * 8u) {
        return static_cast<IntegerType>(code);
    } else {
        RawType mask = (static_cast<RawType>(1u) << bitsNumber) - 1u;
        code &= mask;

        RawType sign_bit = static_cast<RawType>(1u) << (bitsNumber - 1u);

        if (code & sign_bit) {
            RawType extend_mask = ~mask;
            code |= extend_mask;
        }

        return static_cast<IntegerType>(code);
    }
}

// constexpr uint32_t getBitField(uint32_t word, uint8_t first, uint8_t last)
// {
//     if((last - first + 1u) == 32u) return word;
//     return static_cast<uint32_t>( (word >> first) & ( (1u << (last - first + 1u)) - 1u));
// }

template <typename WordType>
WordType getBitField(WordType word, uint8_t first, uint8_t length)
{
    if (length == sizeof(WordType) * 8u) {
        return word;
    }

    WordType mask = (static_cast<WordType>(1u) << length) - 1u;
    return (word >> first) & mask;
}

namespace string_utils
{
template <typename... Args>
std::string concatenate(Args... args)
{
    std::string res;
    (res.append(args), ...);
    return res;
}
} // namespace string_utils



#endif