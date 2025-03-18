#include "utils.h"

#include <library/cpp/testing/unittest/registar.h>

std::pair<std::string, time_t>
TUtils::CreateFreshNoAuthSession() {
    return CreateFreshNoAuthSession(0);
}

std::pair<std::string, time_t>
TUtils::CreateFreshSession(const std::string& subcookie) {
    return CreateFreshSession(subcookie, 0);
}

std::pair<std::string, time_t>
TUtils::CreateFreshSessionV2(const std::string& subcookie) {
    return CreateFreshSessionV2(subcookie, 0);
}

std::pair<std::string, time_t>
TUtils::CreateFreshSessionV3(const std::string& subcookie) {
    return CreateFreshSessionV3(subcookie, 0);
}

std::pair<std::string, time_t>
TUtils::CreateFreshSessionV4(const std::string& subcookie) {
    return CreateFreshSessionV4(subcookie, 0);
}

std::pair<std::string, time_t>
TUtils::CreateFreshNoAuthSession(time_t delta) {
    std::pair<std::string, time_t> result;
    result.second = time(nullptr) - delta;
    result.first = "noauth:" + std::to_string(result.second);
    return result;
}

std::pair<std::string, time_t>
TUtils::CreateFreshSession(const std::string& subcookie, time_t delta) {
    std::pair<std::string, time_t> result;
    result.second = time(nullptr) - delta;
    result.first = std::to_string(result.second) + subcookie;
    return result;
}

std::pair<std::string, time_t>
TUtils::CreateFreshSessionV2(const std::string& subcookie, time_t delta) {
    std::pair<std::string, time_t> result;
    result.second = time(nullptr) - delta;
    result.first = std::string("2:") + std::to_string(result.second) + subcookie;
    return result;
}

std::pair<std::string, time_t>
TUtils::CreateFreshSessionV3(const std::string& subcookie, time_t delta) {
    std::pair<std::string, time_t> result;
    result.second = time(nullptr) - delta;
    result.first = std::string("3:") + std::to_string(result.second) + subcookie;
    return result;
}

std::pair<std::string, time_t>
TUtils::CreateFreshSessionV4(const std::string& subcookie, time_t delta) {
    std::pair<std::string, time_t> result;
    result.second = time(nullptr) - delta;
    result.first = std::string("4:") + std::to_string(result.second) + subcookie;
    return result;
}
