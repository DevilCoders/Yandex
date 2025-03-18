#pragma once

#include <tuple>
#include <utility>

#include <util/generic/function.h>

#include <library/cpp/offroad/utility/finisher.h>

namespace NOffroad {
    template <class KeyDataFactory, class HitWriter, class KeyWriter>
    class TKeyInvWriter {
        using TKeyData = typename KeyWriter::TKeyData;
        using TFinisher = NOffroad::TFinisher<HitWriter, KeyWriter>;
        using TFinishType = typename TFinisher::result_type;

    public:
        using THit = typename HitWriter::THit;
        using TKey = typename KeyWriter::TKey;
        using TKeyRef = typename KeyWriter::TKeyRef;

        using TKeyDataFactory = KeyDataFactory;
        using THitWriter = HitWriter;
        using TKeyWriter = KeyWriter;
        using THitTable = typename THitWriter::TTable;
        using TKeyTable = typename TKeyWriter::TTable;
        using THitModel = typename THitWriter::TModel;
        using TKeyModel = typename TKeyWriter::TModel;

        enum {
            Stages = 1
        };

        TKeyInvWriter() {
        }

        template <class... HitArgs, class... KeyArgs>
        TKeyInvWriter(std::tuple<HitArgs...> hitArgs, std::tuple<KeyArgs...> keyArgs) {
            Reset(std::move(hitArgs), std::move(keyArgs));
        }

        ~TKeyInvWriter() {
            Finish();
        }

        template <class... HitArgs, class... KeyArgs>
        void Reset(std::tuple<HitArgs...> hitArgs, std::tuple<KeyArgs...> keyArgs) {
            Apply(MakeResetter(&HitWriter_), std::move(hitArgs));
            Apply(MakeResetter(&KeyWriter_), std::move(keyArgs));
            PrepareNext();
        }

        void WriteHit(const THit& hit) {
            KeyDataFactory::AddHit(hit, &KeyData_);

            HitWriter_.WriteHit(hit);
        }

        void WriteKey(const TKeyRef& key) {
            /* Write key only if we actually had hits in it. */
            if (HitWriter_.Position() != KeyHitsStart_) {
                KeyDataFactory::SetEnd(HitWriter_.Position(), &KeyData_);
                KeyWriter_.WriteKey(key, KeyData_);
            }

            PrepareNext();
        }

        void WriteLayer() {
        }

        TFinishType Finish() {
            if (IsFinished()) {
                return TFinishType();
            } else {
                return TFinisher()(&HitWriter_, &KeyWriter_);
            }
        }

        bool IsFinished() const {
            return HitWriter_.IsFinished();
        }

    private:
        void PrepareNext() {
            KeyData_ = TKeyData();
            KeyHitsStart_ = HitWriter_.Position();
            HitWriter_.WriteSeekPoint();
        }

    private:
        THitWriter HitWriter_;
        TKeyWriter KeyWriter_;
        TKeyData KeyData_;
        TDataOffset KeyHitsStart_;
    };

}
