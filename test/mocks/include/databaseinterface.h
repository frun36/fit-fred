#pragma once

#include <string>
#include <type_traits>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <memory>

struct MultiBase {
    virtual bool isString() const = 0;
    virtual bool isDouble() const = 0;
    virtual bool isInt() const = 0;
    bool isNumeric() const { return isDouble() || isInt(); }

    virtual std::string getString() const = 0;
    virtual double getDouble() const = 0;
    virtual int getInt() const = 0;

    virtual ~MultiBase() = default;
};

template <typename T>
struct Box : public MultiBase {
    T value;

    Box(T value);

    constexpr bool isString() const override;
    constexpr bool isDouble() const override;
    constexpr bool isInt() const override;

    std::string getString() const override;
    double getDouble() const override;
    int getInt() const override;
};

struct DatabaseInterface {
    static std::unordered_map<std::string, std::vector<std::vector<MultiBase*>>> s_queryResults;

    static bool isConnected();
    static std::vector<std::vector<MultiBase*>> executeQuery(const std::string& query);
};
