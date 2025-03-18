#pragma once

#include <functional>
#include <tuple>

#include <util/generic/function.h>

#include <library/cpp/offroad/utility/tagged.h>
#include <library/cpp/offroad/utility/resetter.h>
#include <library/cpp/offroad/utility/materialize.h>
#include <library/cpp/offroad/tuple/seek_mode.h>

#include "keyinv_iterator.h"
#include "keyinv_searcher_cache.h"

namespace NOffroad {
    template <class KeyDataFactory, class HitReader, class KeyReader, class KeySeeker>
    class TKeyInvSearcher {
    public:
        using THit = typename HitReader::THit;
        using TKey = typename KeyReader::TKey;
        using TKeyRef = typename KeyReader::TKeyRef;
        using TKeyData = typename KeyReader::TKeyData;
        using TIterator = TKeyInvIterator<HitReader, KeyReader>;

        using THitTable = typename HitReader::TTable;
        using TKeyTable = typename KeyReader::TTable;
        using THitModel = typename HitReader::TModel;
        using TKeyModel = typename KeyReader::TModel;

        TKeyInvSearcher()
            : Cache_(new TKeyInvSearcherCache<TKeyData>())
        {
        }

        template <class... HitArgs, class... KeyArgs, class... SeekerArgs>
        TKeyInvSearcher(std::tuple<HitArgs...> hitArgs, std::tuple<KeyArgs...> keyArgs, std::tuple<SeekerArgs...> seekerArgs)
            : Cache_(new TKeyInvSearcherCache<TKeyData>())
        {
            Reset(std::move(hitArgs), std::move(keyArgs), std::move(seekerArgs));
        }

        template <class... HitArgs, class... KeyArgs, class... SeekerArgs>
        void Reset(std::tuple<HitArgs...> hitArgs, std::tuple<KeyArgs...> keyArgs, std::tuple<SeekerArgs...> seekerArgs) {
            Apply(MakeResetter(&Seeker_), std::move(seekerArgs));

            auto storedHitArgs = Materialize(std::move(hitArgs));
            auto storedKeyArgs = Materialize(std::move(keyArgs));
            IteratorResetter_ = [=](TIterator* iterator) {
                Apply(MakeResetter(&iterator->HitReader_), storedHitArgs);
                Apply(MakeResetter(&iterator->KeyReader_), storedKeyArgs);
            };

            Cache_->Clear();
        }

        bool Find(const TKeyRef& key, TIterator* iterator) const {
            if (iterator->Tag() != this) {
                iterator->SetTag(this);
                IteratorResetter_(iterator);
            }

            TKeyData start, end;
            if (!FindData(key, &start, &end, &iterator->KeyReader_))
                return false;

            if (!iterator->HitReader_.Seek(KeyDataFactory::End(start), THit(), TSeekPointSeek()))
                return false;

            iterator->HitReader_.SetLimits(start, end);
            return true;
        }

    private:
        bool FindData(const TKeyRef& key, TKeyData* start, TKeyData* end, KeyReader* reader) const {
            if (auto entry = Cache_->Entry(key)) {
                if (entry->Key() == key) {
                    if (!entry->Status())
                        return false;

                    *start = entry->Start();
                    *end = entry->End();
                    return true;
                } else {
                    auto unlocked = entry.get();
                    entry.reset();

                    bool status = FindDataInIndex(key, start, end, reader);

                    if (entry = unlocked->TryLock()) {
                        entry->SetKey(key);
                        entry->SetStart(*start);
                        entry->SetEnd(*end);
                        entry->SetStatus(status);
                    }

                    return status;
                }
            } else {
                return FindDataInIndex(key, start, end, reader);
            }
        }

        bool FindDataInIndex(const TKeyRef& key, TKeyData* start, TKeyData* end, KeyReader* reader) const {
            TKeyRef localKey;
            TKeyData localData;
            if (!Seeker_.LowerBound(key, &localKey, &localData, reader))
                return false;

            if (key != localKey)
                return false;

            *end = localData;
            *start = reader->LastData();
            return true;
        }

    private:
        KeySeeker Seeker_;
        std::function<void(TIterator*)> IteratorResetter_;
        THolder<TKeyInvSearcherCache<TKeyData>> Cache_;
    };

}
