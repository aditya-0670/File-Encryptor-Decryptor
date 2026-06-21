#ifndef READ_ENV_HPP
#define READ_ENV_HPP

#include <optional>
#include <string>

class ReadEnv {
public:
    static std::optional<std::string> readPassword(std::string& error, const std::string& envPath = ".env");
};

#endif
