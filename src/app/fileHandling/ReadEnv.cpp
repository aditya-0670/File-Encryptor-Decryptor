#include "ReadEnv.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

namespace {

std::string trim(std::string value) {
    auto isSpace = [](unsigned char ch) {
        return std::isspace(ch);
    };

    value.erase(value.begin(), std::find_if_not(value.begin(), value.end(), isSpace));
    value.erase(std::find_if_not(value.rbegin(), value.rend(), isSpace).base(), value.end());
    return value;
}

std::optional<std::string> extractPasswordText(const std::string& content) {
    std::istringstream lines(content);
    std::string line;
    std::optional<std::string> rawPassword;

    while (std::getline(lines, line)) {
        const std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        const std::size_t delimiter = trimmed.find('=');
        if (delimiter == std::string::npos) {
            if (!rawPassword.has_value()) {
                rawPassword = trimmed;
            }
            continue;
        }

        const std::string name = trim(trimmed.substr(0, delimiter));
        if (name == "VAULT_PASSWORD" || name == "PASSWORD" || name == "KEY") {
            return trim(trimmed.substr(delimiter + 1));
        }
    }

    return rawPassword;
}

} // namespace

std::optional<std::string> ReadEnv::readPassword(std::string& error, const std::string& envPath) {
    std::ifstream input(envPath);
    if (!input.is_open()) {
        error = "missing .env file; create one containing VAULT_PASSWORD=<password>";
        return std::nullopt;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();

    std::optional<std::string> password = extractPasswordText(buffer.str());
    if (!password.has_value() || password->empty()) {
        error = "missing password in .env; expected VAULT_PASSWORD=<password>";
        return std::nullopt;
    }

    return password;
}
