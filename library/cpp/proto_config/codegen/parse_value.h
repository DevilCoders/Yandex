#pragma once

#include <util/string/cast.h>
#include <util/generic/maybe.h>

namespace NProtoConfig {
    template <typename T>
    void ParseConfigValue(const T& config, T& value) {
        value = config;
    }

    template <typename T>
    void ParseConfigValue(const T& config, TMaybe<T>& value) {
        value = config;
    }

    template <typename T>
    void ParseConfigValue(TStringBuf config, T& value) {
        if (config) {
            value = FromString<T>(config);
        } else {
            value = T();
        }
    }

    template <typename V, typename T>
    V ParseConfigValue(const T& config) {
        V value;
        ParseConfigValue(config, value);
        return value;
    }
}
