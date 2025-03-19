#pragma once

#include "hits_serializer.h"

#include <kernel/text_machine/interface/hit.h>
#include <kernel/reqbundle/reqbundle.h>

#include <util/generic/map.h>
#include <util/generic/deque.h>
#include <util/generic/vector.h>
#include <util/generic/maybe.h>


namespace NTextMachineProtocol {
    class TPbDocHits;
} // NTextMachineProtocol

namespace NTextMachine {
    class TExpansionsResolver;

    class THitsMap {
    public:
        enum EHitType {
            Exact,
            Lemma,
            Synonym
        };

        class TSentenceType {
        private:
            EExpansionType ExpansionType = TExpansion::ExpansionMax;
            EStreamType StreamType = TStream::StreamMax;

        public:
            TSentenceType() = default;

            TSentenceType(EExpansionType expType)
                : ExpansionType(expType)
            {}

            TSentenceType(EStreamType strType)
                : StreamType(strType)
            {}

            bool operator< (const TSentenceType& other) const {
                return std::forward_as_tuple(ExpansionType, StreamType)
                    < std::forward_as_tuple(other.ExpansionType, other.StreamType);
            }

            bool IsStream() const {
                return StreamType != TStream::StreamMax;
            }

            bool IsExpansion() const {
                return ExpansionType != TExpansion::ExpansionMax;
            }

            EExpansionType AsExpansion() const {
                Y_ENSURE(IsExpansion());
                return ExpansionType;
            }

            EStreamType AsStream() const {
                Y_ENSURE(IsStream());
                return StreamType;
            }

            bool Valid() const {
                return IsStream() != IsExpansion();
            }
        };

        struct TWordsRange {
            size_t Begin = 0;
            size_t End = 0; // inclusive

            bool operator< (const TWordsRange& other) const {
                return std::forward_as_tuple(Begin, End)
                    < std::forward_as_tuple(other.Begin, other.End);
            }

            bool Contains(size_t index) const {
                return Begin <= index && index <= End;
            }
        };

        class THit {
        private:
            friend class THitsMap;

            const NTextMachine::TBlockHit* BlockHit = nullptr;

            EHitType Type = EHitType::Exact;

            TSentenceType DualType;
            float DualValue = 0.0f;
            size_t DualIdx = 0;
            size_t DualWordBegin = 0;
            size_t DualWordEnd = 0;
            size_t DualLength = 0;

        public:
            const NTextMachine::TBlockHit& BlockHitRef() const {
                Y_ENSURE(BlockHit);
                return *BlockHit;
            }

            EHitType GetType() const {
                return Type;
            }

            size_t GetMatchedIndex() const {
                return DualIdx;
            }

            TWordsRange GetMatchedWordsRange() const {
                return {DualWordBegin, DualWordEnd};
            }

            float GetMatchedValue() const {
                return DualValue;
            }

            TSentenceType GetMatchedType() const {
                return DualType;
            }

            size_t GetMatchedLength() const {
                return DualLength;
            }
        };

        struct TWord {
            TVector<THit> Hits;

            bool Empty() const {
                return Hits.empty();
            }
        };

        struct TSentence {
            float Value = 0.0f;

            TVector<TWord> Words;
        };

        struct TSentenceId {
            TSentenceId(ui32 id, TSentenceType sentenceType)
                : Id(id)
                , SentenceType(sentenceType)
            {
            }

            bool operator< (const TSentenceId& other) const {
                return std::forward_as_tuple(Id, SentenceType)
                    < std::forward_as_tuple(other.Id, other.SentenceType);
            }

            ui32 Id = 0;
            TSentenceType SentenceType;
        };

        struct TDoc {
            TMap<TSentenceId, TSentence> Sentences;

            struct TStorage {
                TDeque<NTextMachine::TBlockHit> Hits;
                THolder<NTextMachine::THitsAuxData> AuxData;
            };

            TReqBundle Bundle;
            TStorage Storage;
        };

        // in the following methods empty streamTypes argument
        // means "any stream"

        static THolder<TDoc> CreateForDoc(
            const NTextMachineProtocol::TPbDocHits& hits,
            const TMaybe<NLingBoost::EExpansionType>& expansionType,
            const TSet<NLingBoost::EStreamType>& streamTypes);

        static THolder<TDoc> CreateForRequest(
            const NTextMachineProtocol::TPbDocHits& hits,
            const TMaybe<NLingBoost::EExpansionType>& expansionType,
            const TSet<NLingBoost::EStreamType>& streamTypes);

        static TVector<TString> GetRequestWords(const THolder<TDoc>& doc, const size_t requestIndex);

    private:
        static ui32 ResolveBreakNumber(EStreamType streamType, ui32 breakNumber) {
            if (streamType == NLingBoost::EStreamType::Body) {
                //avoid body and url hits mixing
                //https://a.yandex-team.ru/arc/trunk/arcadia/search/lingboost/hits_streamizer/rt_hits.cpp?rev=3848991#L121
                return breakNumber + 1;
            }
            return breakNumber;
        }

        static void AppendHitForDoc(
            THitsMap::TDoc& doc,
            const NTextMachine::TBlockHit& blockHit,
            TMaybe<NLingBoost::EExpansionType> expansionType,
            TExpansionsResolver& resolver);

        static void AppendHitForRequest(
            THitsMap::TDoc& doc,
            const NTextMachine::TBlockHit& blockHit,
            TMaybe<NLingBoost::EExpansionType> expansionType,
            TExpansionsResolver& resolver);
    };
} // NTextMachine
