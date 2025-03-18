#pragma once

#include <tuple>
#include <utility>

#include <util/generic/function.h>

#include <library/cpp/offroad/utility/resetter.h>
#include <library/cpp/offroad/tuple/seek_mode.h>

namespace NOffroad {
    template <class KeyDataFactory, class HitReader, class KeyReader>
    class TKeyInvReader {
        using THitReader = HitReader;
        using TKeyReader = KeyReader;

    public:
        using THit = typename THitReader::THit;
        using TKey = typename TKeyReader::TKey;
        using TKeyRef = typename TKeyReader::TKeyRef;
        using TKeyData = typename TKeyReader::TKeyData;

        using THitTable = typename THitReader::TTable;
        using TKeyTable = typename TKeyReader::TTable;
        using THitModel = typename THitReader::TModel;
        using TKeyModel = typename TKeyReader::TModel;

        TKeyInvReader() {
        }

        template <class... HitArgs, class... KeyArgs>
        TKeyInvReader(std::tuple<HitArgs...> hitArgs, std::tuple<KeyArgs...> keyArgs) {
            Reset(std::move(hitArgs), std::move(keyArgs));
        }

        template <class... HitArgs, class... KeyArgs>
        void Reset(std::tuple<HitArgs...> hitArgs, std::tuple<KeyArgs...> keyArgs) {
            Apply(MakeResetter(&HitReader_), std::move(hitArgs));
            Apply(MakeResetter(&KeyReader_), std::move(keyArgs));
            ResetHitLimits();
        }

        void Restart() {
            HitReader_.Restart();
            KeyReader_.Restart();
            ResetHitLimits();
        }

        bool ReadKey(TKeyRef* key) {
            TKeyData data;
            return ReadKey(key, &data);
        }

        bool ReadKey(TKeyRef* key, TKeyData* data) {
            TKeyData lastData = KeyReader_.LastData();
            if (!KeyReader_.ReadKey(key, data)) {
                ResetHitLimits();
                return false;
            }

            HitReader_.Seek(KeyDataFactory::End(lastData), THit(), TSeekPointSeek());
            HitReader_.SetLimits(KeyDataFactory::End(lastData), KeyDataFactory::End(*data));
            return true;
        }

        Y_FORCE_INLINE bool ReadHit(THit* hit) {
            return HitReader_.ReadHit(hit);
        }

        bool LowerBound(const TKeyRef& key) {
            if (!KeyReader_.LowerBound(key))
                return false;

            ResetHitLimits();
            return true;
        }

    private:
        void ResetHitLimits() {
            HitReader_.SetLimits(HitReader_.Position(), HitReader_.Position());
        }

    private:
        THitReader HitReader_;
        TKeyReader KeyReader_;
    };

}
