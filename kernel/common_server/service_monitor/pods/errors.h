#pragma once

#include <library/cpp/json/json_value.h>
#include <library/cpp/threading/future/core/future.h>

namespace NServiceMonitor {

    NJson::TJsonValue SerializeToJson(std::exception_ptr exception) noexcept;

    template<class TFuture>
    NJson::TJsonValue SerializeFutureExceptionToJson(const TFuture& future) {
        NJson::TJsonValue result;
        try {
            future.TryRethrow();
        } catch (...) {
            result = SerializeToJson(std::current_exception());
        }
        return result;
    }

}
