#pragma once

#include <iostream>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace VE {

// Write() is a helper for printing container types to the std::ostream.
// Write() is used by the LOG() macro below.

// Forward declarations
inline std::ostream& Write(std::ostream& out) {
    return out;
}

template <typename T>
inline std::ostream& Write(std::ostream& out, const std::optional<T>& value);
template <typename T>
inline std::ostream& Write(std::ostream& out, const std::vector<T>& value);
template <typename K, typename V>
inline std::ostream& Write(std::ostream& out, const std::unordered_map<K, V>& value);
template <typename... TYS>
inline std::ostream& Write(std::ostream& out, const std::variant<TYS...>& value);
template <typename VALUE>
inline std::ostream& Write(std::ostream& out, VALUE&& value);

template <typename T>
std::ostream& Write(std::ostream& out, const std::optional<T>& value) {
    if (value.has_value()) {
        return Write(out, value.value());
    }
    return out << "<undefined>";
}

template <typename T>
std::ostream& Write(std::ostream& out, const std::vector<T>& value) {
    out << "[";
    bool first = true;
    for (const auto& el : value) {
        if (!first) {
            out << ", ";
        }
        first = false;
        Write(out, el);
    }
    return out << "]";
}

template <typename K, typename V>
std::ostream& Write(std::ostream& out, const std::unordered_map<K, V>& value) {
    out << "{";
    bool first = true;
    for (auto& [key, val] : value) {
        if (!first) {
            out << ", ";
        }
        first = false;
        Write(out, key);
        out << ": ";
        Write(out, val);
    }
    return out << "}";
}

template <typename... TYS>
std::ostream& Write(std::ostream& out, const std::variant<TYS...>& value) {
    std::visit([&](auto&& v) { Write(out, v); }, value);
    return out;
}

template <typename VALUE>
std::ostream& Write(std::ostream& out, VALUE&& value) {
    return out << std::forward<VALUE>(value);
}

template <typename FIRST, typename... REST>
inline std::ostream& Write(std::ostream& out, FIRST&& first, REST&&... rest) {
    Write(out, std::forward<FIRST>(first));
    Write(out, std::forward<REST>(rest)...);
    return out;
}

// Fatal() prints a message to stdout with the given file, line, function and optional message,
// then calls abort(). Fatal() is usually not called directly, but by the UNREACHABLE() and
// UNIMPLEMENTED() macro below.
template <typename... MSG_ARGS>
[[noreturn]] inline void Fatal(const char* reason,
                               const char* file,
                               int line,
                               const char* function,
                               MSG_ARGS&&... msg_args) {
    std::stringstream msg;
    msg << file << ":" << line << ": " << reason << ": " << function << "()";
    if constexpr (sizeof...(msg_args) > 0) {
        msg << " ";
        Write(msg, std::forward<MSG_ARGS>(msg_args)...);
    }
    std::cout << msg.str() << "\n";
    abort();
}

// LOG() prints the current file, line and function to stdout, followed by a
// string representation of all the variadic arguments.
#define LOG(...)                                                                   \
    Write(std::cout << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << ": ", \
          __VA_ARGS__)                                                             \
    << "\n"

#define UNREACHABLE(...) \
    Fatal("UNREACHABLE", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

} // namespace VE