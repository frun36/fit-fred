#ifndef UTILS_FIT
#define UTILS_FIT

#include <cmath>
#include <cstdint>
#include <functional>

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

template <typename OkType, typename ErrorType>
struct Result {
    std::optional<OkType> ok;
    std::optional<ErrorType> error;

    bool isOk()
    {
        return !error.has_value();
    }
};

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

class Splitter
{
   public:
    Splitter(std::string_view sequence_, char delimiter_)
        : sequence(sequence_), delimiter(delimiter_), currentStart(0), currentEnd(0) {}

    std::string_view getNext()
    {
        if (reachedEnd()) {
            throw std::out_of_range("Reached end of sequence");
        }

        size_t pos = sequence.find(delimiter, currentStart);
        size_t tokenEnd = (pos == std::string::npos) ? sequence.size() : pos;
        std::string_view token = sequence.substr(currentStart, tokenEnd - currentStart);
        if (pos == std::string::npos) {
            currentStart = sequence.size();
        } else {
            currentStart = pos + 1;
        }

        return token;
    }

    std::string_view getNext(size_t& pos)
    {
        if (reachedEnd()) {
            throw std::out_of_range("Reached end of sequence");
        }

        pos = sequence.find(delimiter, currentStart);
        size_t tokenEnd = (pos == std::string::npos) ? sequence.size() : pos;
        std::string_view token = sequence.substr(currentStart, tokenEnd - currentStart);
        if (pos == std::string::npos) {
            currentStart = sequence.size();
        } else {
            currentStart = pos + 1;
        }

        return token;
    }

    static Result<std::vector<std::string>, std::string> getAll(std::string_view sequence_, char delimiter_)
    {
        Splitter splitter(sequence_, delimiter_);
        std::vector<std::string> substrings;
        try {
            while (splitter.reachedEnd() == false) {
                std::string_view next = splitter.getNext();
                substrings.emplace_back(next, 0, next.size());
            }
        } catch (std::exception& e) {
            return { .ok = std::nullopt, .error = e.what() };
        }
        return { .ok = std::move(substrings), .error = std::nullopt };
    }

    bool reachedEnd() const
    {
        return sequence.empty() || currentStart >= sequence.size();
    }

    void reset()
    {
        currentStart = 0;
        currentEnd = 0;
    }

   private:
    std::string_view sequence;
    const char delimiter;
    size_t currentStart;
    size_t currentEnd;
};

} // namespace string_utils

#endif