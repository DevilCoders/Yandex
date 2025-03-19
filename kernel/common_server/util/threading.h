#pragma once
#include <util/generic/string.h>
#include <library/cpp/threading/future/core/future.h>

namespace NThreading {
    template <class T>
    TString GetExceptionMessage(const TFuture<T>& future) {
        try {
            future.GetValue();
            return {};
        } catch (const std::exception& e) {
            return e.what();
        } catch (...) {
            return "unknown exception";
        }
    }
}
