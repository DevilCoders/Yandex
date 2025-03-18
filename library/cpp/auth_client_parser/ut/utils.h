#pragma once

#include <ctime>
#include <string>

class TUtils {
public:
    static std::pair<std::string, time_t> CreateFreshNoAuthSession();
    static std::pair<std::string, time_t> CreateFreshSession(const std::string& subcookie);
    static std::pair<std::string, time_t> CreateFreshSessionV2(const std::string& subcookie);
    static std::pair<std::string, time_t> CreateFreshSessionV3(const std::string& subcookie);
    static std::pair<std::string, time_t> CreateFreshSessionV4(const std::string& subcookie);

    static std::pair<std::string, time_t> CreateFreshNoAuthSession(time_t delta);
    static std::pair<std::string, time_t> CreateFreshSession(const std::string& subcookie, time_t delta);
    static std::pair<std::string, time_t> CreateFreshSessionV2(const std::string& subcookie, time_t delta);
    static std::pair<std::string, time_t> CreateFreshSessionV3(const std::string& subcookie, time_t delta);
    static std::pair<std::string, time_t> CreateFreshSessionV4(const std::string& subcookie, time_t delta);
};
