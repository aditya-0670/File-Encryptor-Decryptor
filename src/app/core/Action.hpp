#ifndef VAULT_CORE_ACTION_HPP
#define VAULT_CORE_ACTION_HPP

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>

namespace vault {

enum class Action {
    Encrypt,
    Decrypt
};

inline std::string trim(std::string value) {
    auto isSpace = [](unsigned char ch) {
        return std::isspace(ch);
    };

    value.erase(value.begin(), std::find_if_not(value.begin(), value.end(), isSpace));
    value.erase(std::find_if_not(value.rbegin(), value.rend(), isSpace).base(), value.end());
    return value;
}

inline std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

inline std::optional<Action> parseAction(const std::string& value) {
    const std::string normalized = toLower(trim(value));
    if (normalized == "encrypt") {
        return Action::Encrypt;
    }
    if (normalized == "decrypt") {
        return Action::Decrypt;
    }
    return std::nullopt;
}

inline std::string actionToString(Action action) {
    return action == Action::Encrypt ? "ENCRYPT" : "DECRYPT";
}

} // namespace vault

#endif
