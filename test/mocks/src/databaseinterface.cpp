#include "../include/databaseinterface.h"

// Implementing Box template methods

template <typename T>
Box<T>::Box(T value) : value(value) {}

template <typename T>
constexpr bool Box<T>::isString() const
{
    return std::is_same<T, std::string>::value;
}

template <typename T>
constexpr bool Box<T>::isDouble() const
{
    return std::is_same<T, double>::value;
}

template <typename T>
constexpr bool Box<T>::isInt() const
{
    return std::is_same<T, int>::value;
}

template <typename T>
std::string Box<T>::getString() const
{
    if constexpr (!std::is_same<T, std::string>::value)
        throw std::runtime_error("DB mock: getString called for non-string type");
    else
        return value;
}

template <typename T>
double Box<T>::getDouble() const
{
    if constexpr (std::is_same<T, double>::value) {
        return value;
    } else if constexpr (std::is_arithmetic_v<T>) {
        return static_cast<double>(value);
    } else {
        throw std::runtime_error("DB mock: getDouble called for non-numeric type");
    }
}

template <typename T>
int Box<T>::getInt() const
{
    if constexpr (std::is_same<T, int>::value) {
        return value;
    } else if constexpr (std::is_arithmetic_v<T>) {
        return static_cast<int>(value);
    } else {
        throw std::runtime_error("DB mock: getInt called for non-numeric type");
    }
}

// Initialize static member
std::unordered_map<std::string, std::vector<std::vector<MultiBase*>>> DatabaseInterface::s_queryResults;

bool DatabaseInterface::isConnected()
{
    return true;
}

std::vector<std::vector<MultiBase*>> DatabaseInterface::executeQuery(const std::string& query)
{
    if (s_queryResults.count(query) == 0)
        throw std::runtime_error("DB mock: invalid query");
    return s_queryResults[query];
}

// Ensure the template code is compiled by providing the possible instantiations
template class Box<std::string>;
template class Box<int>;
template class Box<double>;