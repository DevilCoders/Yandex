#pragma once

#include <util/system/mutex.h>
#include <util/generic/hash.h>
#include <util/generic/singleton.h>
#include <util/memory/blob.h>

#include <type_traits>

namespace NDoom {
    template <typename F>
    class TDeduplicator {
        using T = typename std::result_of_t<decltype(&F::Create)(const TBlob&)>::TValueType;
    public:
        bool DeduplicationEnabled = false;

        TAtomicSharedPtr<const T> GetOrCreate(const TBlob& blob) {
            if (!DeduplicationEnabled) {
                return F::Create(blob);
            }

            with_lock (Lock_) {
                TAtomicSharedPtr<const T>& item = Items_[blob.AsStringBuf()];
                if (!item) {
                    item = F::Create(blob);
                }

                return item;
            }
        }

    private:
        TMutex Lock_;
        THashMap<TString, TAtomicSharedPtr<const T>> Items_;
    };

    template <typename T>
    TDeduplicator<T>& Deduplicator() {
        return *Singleton<TDeduplicator<T>>();
    }
}
