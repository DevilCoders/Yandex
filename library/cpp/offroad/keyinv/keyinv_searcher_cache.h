#pragma once

#include <array>
#include <memory>

#include <util/generic/strbuf.h>
#include <util/digest/murmur.h>
#include <library/cpp/deprecated/atomic/atomic.h>

namespace NOffroad {
    namespace NPrivate {
        struct TEntryUnlocker {
            template <class Entry>
            void operator()(Entry* entry) const {
                if (entry)
                    entry->Unlock();
            }
        };
    }

    template <class KeyData>
    class TKeyInvSearcherCacheEntry {
    public:
        using TLockedPtr = std::unique_ptr<TKeyInvSearcherCacheEntry<KeyData>, NPrivate::TEntryUnlocker>;
        using TKeyData = KeyData;

        enum {
            /**
             * A short note on why max size is what it is. We're using per-entry
             * spinlocks to ensure thread-safety of our cache, and in a multi-processor
             * environment (read: on all modern PCs) those operate on cache lines.
             *
             * So to reduce bus load it makes sense to place no more than one entry
             * per cache line. Given cache line size of 64, this is the number we get.
             *
             * Tests on WebTier0 panther index show that only 0.2% of keys are longer.
             * This means that in normal index the number will be even lower as there
             * are no bigrams there.
             */
            MaxKeySize = 64 - 2 - sizeof(TAtomic)
        };

        TKeyInvSearcherCacheEntry() {
        }

        TLockedPtr TryLock() {
            if (AtomicTryLock(&Lock_)) {
                return TLockedPtr(this);
            } else {
                return TLockedPtr(nullptr);
            }
        }

        void Unlock() {
            AtomicUnlock(&Lock_);
        }

        TStringBuf Key() const {
            return TStringBuf(Key_.data(), KeySize_);
        }

        void SetKey(const TStringBuf& key) {
            Y_ASSERT(key.size() <= MaxKeySize);

            KeySize_ = key.size();
            memcpy(Key_.data(), key.data(), key.size());
        }

        bool Status() const {
            return Status_;
        }

        void SetStatus(bool status) {
            Status_ = status;
        }

        const TKeyData& Start() const {
            return Start_;
        }

        void SetStart(const TKeyData& start) {
            Start_ = start;
        }

        const TKeyData& End() const {
            return End_;
        }

        void SetEnd(const TKeyData& end) {
            End_ = end;
        }

        void Clear() {
            KeySize_ = 0;
        }

    private:
        TAtomic Lock_ = 0;
        ui8 KeySize_ = 0;
        std::array<char, MaxKeySize> Key_;
        bool Status_ = false;
        TKeyData Start_;
        TKeyData End_;
    };

    template <class KeyData>
    class TKeyInvSearcherCache {
    public:
        using TEntry = TKeyInvSearcherCacheEntry<KeyData>;
        using TLockedEntryPtr = typename TEntry::TLockedPtr;

        enum {
            Size = 8192
        };

        /**
         * Thread-safe.
         */
        TLockedEntryPtr Entry(const TStringBuf& key) {
            if (key.size() > TEntry::MaxKeySize)
                return nullptr;

            size_t index = MurmurHash<ui64>(key.data(), key.size()) % Size;

            return Cache_[index].TryLock();
        }

        /**
         * Not thread-safe.
         */
        void Clear() {
            for (TEntry& entry : Cache_)
                entry.Clear();
        }

    private:
        std::array<TEntry, Size> Cache_;
    };

}
