#pragma once

#include "request_contents.h"
#include "reqbundle_fwd.h"

#include <kernel/idf/idf.h>

#include <library/cpp/packedtypes/packedfloat.h>

#include <util/generic/xrange.h>

namespace NReqBundle {
    template <typename DataType>
    class TProxAccBase
        : public NDetail::TLightAccessor<DataType>
    {
    public:
        using NDetail::TLightAccessor<DataType>::Contents;

        TProxAccBase() = default;
        TProxAccBase(const TProxAcc& other)
            : NDetail::TLightAccessor<DataType>(other.Contents())
        {}
        TProxAccBase(const TConstProxAcc& other)
            : NDetail::TLightAccessor<DataType>(other.Contents())
        {}
        TProxAccBase(DataType& data)
            : NDetail::TLightAccessor<DataType>(data)
        {}

        float GetCohesion() const {
            return Contents().Cohesion;
        }
        void SetCohesion(float value) const {
            Y_ASSERT(0.0f <= value && value <= 1.0f);
            Contents().Cohesion = value;
        }

        bool IsMultitoken() const {
            return Contents().Multitoken;
        }
        void SetMultitoken(bool value) const {
            Contents().Multitoken = value;
        }
    };

    template <typename DataType>
    class TRequestWordAccBase
        : public NDetail::TLightAccessor<DataType>
    {
    public:
        using NDetail::TLightAccessor<DataType>::Contents;

        TRequestWordAccBase() = default;
        TRequestWordAccBase(const TRequestWordAcc& other)
            : NDetail::TLightAccessor<DataType>(other.Contents())
        {}
        TRequestWordAccBase(const TConstRequestWordAcc& other)
            : NDetail::TLightAccessor<DataType>(other.Contents())
        {}
        TRequestWordAccBase(DataType& data)
            : NDetail::TLightAccessor<DataType>(data)
        {}

        // Compound reverse frequencies that
        // aggregate all matches per word
        float GetIdf(NLingBoost::EWordFreqType type = NLingBoost::TRevFreq::Default) const {
            return ReverseFreq2Idf(GetRevFreq(type));
        }
        long GetRevFreq(NLingBoost::EWordFreqType type = NLingBoost::TRevFreq::Default) const {
            return Contents().RevFreq.Values[type];
        }
        void SetRevFreq(NLingBoost::EWordFreqType type, long value) const {
            Y_ASSERT(NLingBoost::InvalidRevFreq == value || NLingBoost::IsValidRevFreq(value));
            Contents().RevFreq.Values[type] = value;
        }
        void SetRevFreq(long value) const {
            SetRevFreq(NLingBoost::TRevFreq::Default, value);
        }
        const NLingBoost::TRevFreqsByType& GetRevFreqsByType() const {
            return Contents().RevFreq.Values;
        }

        const TString& GetTokenText() const {
            return Contents().Token.GetUtf8();
        }
        const TUtf16String& GetWideTokenText() const {
            return Contents().Token.GetWide();
        }
    };

    template <typename DataType>
    class TMatchAccBase
        : public NDetail::TLightAccessor<DataType>
    {
    public:
        using NDetail::TLightAccessor<DataType>::Contents;

        TMatchAccBase() = default;
        TMatchAccBase(const TMatchAcc& other)
            : NDetail::TLightAccessor<DataType>(other.Contents())
        {}
        TMatchAccBase(const TConstMatchAcc& other)
            : NDetail::TLightAccessor<DataType>(other.Contents())
        {}
        TMatchAccBase(DataType& data)
            : NDetail::TLightAccessor<DataType>(data)
        {}
        TMatchAccBase& operator=(const TMatchAccBase&) = default;

        EMatchType GetType() const {
            return Contents().Type;
        }

        size_t GetBlockIndex() const {
            return Contents().BlockIndex;
        }
        size_t GetWordIndexFirst() const {
            return Contents().WordIndexFirst;
        }
        size_t GetWordIndexLast() const {
            return Contents().WordIndexLast;
        }
        NDetail::TAnchorWordsRange GetAnchorWordsRange() const {
            return Contents().AnchorWordsRange;
        }
        auto GetWordIndexes() const -> decltype(xrange(size_t(0), size_t(0))) {
            return xrange(Contents().WordIndexFirst, Contents().WordIndexLast + 1);
        }
        size_t GetNumWords() const {
            return 1 + Contents().WordIndexLast - Contents().WordIndexFirst;
        }

        // Context-dependent match frequencies
        float GetIdf(NLingBoost::EWordFreqType type = NLingBoost::TRevFreq::Default) const {
            return ReverseFreq2Idf(GetRevFreq(type));
        }
        long GetRevFreq(NLingBoost::EWordFreqType type = NLingBoost::TRevFreq::Default) const {
            return Contents().RevFreq.Values[type];
        }
        void SetRevFreq(NLingBoost::EWordFreqType type, long value) const {
            Y_ASSERT(NLingBoost::InvalidRevFreq == value || NLingBoost::IsValidRevFreq(value));
            Contents().RevFreq.Values[type] = value;
        }
        void SetRevFreq(long value) const {
            SetRevFreq(NLingBoost::TRevFreq::Default, value);
        }
        const NLingBoost::TRevFreqsByType& GetRevFreqsByType() const {
            return Contents().RevFreq.Values;
        }

        // Synonym mask is bit mask;
        // bit values are taken from EThesExtType (proto)
        ui32 GetSynonymMask() const {
            return Contents().SynonymMask;
        }
        void SetSynonymMask(ui32 mask) const {
            Contents().SynonymMask = mask;
        }

        double GetWeight() const {
            return Contents().Weight;
        }
        void SetWeight(double value) const {
            // Y_ASSERT(0.0f <= value && value <= 1.0f);
            // Currently not true
            Contents().Weight = value;
        }
    };

    template <typename DataType>
    class TFacetEntryAccBase
        : public NDetail::TLightAccessor<DataType>
    {
    public:
        using NDetail::TLightAccessor<DataType>::Contents;

        TFacetEntryAccBase() = default;
        TFacetEntryAccBase(const TFacetEntryAcc& other)
            : NDetail::TLightAccessor<DataType>(other.Contents())
        {}
        TFacetEntryAccBase(const TConstFacetEntryAcc& other)
            : NDetail::TLightAccessor<DataType>(other.Contents())
        {}
        TFacetEntryAccBase(DataType& data)
            : NDetail::TLightAccessor<DataType>(data)
        {}

        TFacetId GetId() const {
            return Contents().Id;
        }
        EExpansionType GetExpansion() const {
            return Contents().Id.template Get<EFacetPartType::Expansion>();
        }
        TRegionId GetRegionId() const {
            return Contents().Id.template Get<EFacetPartType::RegionId>();
        }
        ERegionClassType GetRegionClass() const {
            return NLingBoost::GetRegionClassByRegionId(GetRegionId());
        }
        bool IsOriginalRequest() const {
            return GetExpansion() == TExpansion::OriginalRequest || IsGeoOriginalRequest();
        }
        bool IsGeoOriginalRequest() const {
            return GetExpansion() == TExpansion::GeoOriginalRequest || GetExpansion() == TExpansion::GeoWhatOnlyRequest;
        }
        bool IsOriginalRequestSynonyms() const {
            return GetExpansion() == TExpansion::OriginalRequestSynonyms;
        }
        bool NeedTrCompatibilityInfo() const {
            return GetExpansion() == TExpansion::OriginalRequest || GetExpansion() == TExpansion::OriginalRequestSynonyms
                || IsGeoOriginalRequest();
        }

        float GetValue() const {
            return Contents().Value;
        }
        void SetValue(float value) const {
            Y_ASSERT(value >= 0.0f && value <= 1.0f);
            Contents().Value = value;
        }
        template <typename T=ui8>
        void ValueToFrac() const {
            Contents().Value = Frac2Float<T>(Float2Frac<T>(Contents().Value));
        }
    };

    template <typename DataType>
    class TFacetsAccBase
        : public NDetail::TLightAccessor<DataType>
    {
    public:
        using NDetail::TLightAccessor<DataType>::Contents;

        TFacetsAccBase() = default;
        TFacetsAccBase(const TFacetsAcc& other)
            : NDetail::TLightAccessor<DataType>(other.Contents())
        {}
        TFacetsAccBase(const TConstFacetsAcc& other)
            : NDetail::TLightAccessor<DataType>(other.Contents())
        {}
        TFacetsAccBase(DataType& data)
            : NDetail::TLightAccessor<DataType>(data)
        {}

        bool Has(const TFacetId& id) const {
            return !!LookupFacet(Contents(), id);
        }
        bool Lookup(const TFacetId& id, float& value) const {
            if (auto ptr = LookupFacet(Contents(), id)) {
                value = ptr->Value;
                return true;
            }
            return false;
        }
        void Set(const TFacetId& id, float value) const {
            auto res = InsertFacet(Contents(), id);
            res.first.Value = value;
        }
        void UnSet(const TFacetId& id) const {
            RemoveFacet(Contents(), id);
        }
        void UpdateMax(const TFacetId& id, float value) {
            auto res = InsertFacet(Contents(), id);
            if (res.second) {
                res.first.Value = value;
            } else {
                res.first.Value = Max(value, res.first.Value);
            }
        }
        void Clear() const {
            Contents().Entries.clear();
        }

        float GetMaxValue() const {
            float res = 0.0f;
            for (const auto& entry : Contents().Entries) {
                res = Max(res, entry.Value);
            }

            return res;
        }

        size_t GetNumEntries() const {
            return Contents().Entries.size();
        }
        TFacetEntryAcc Entry(size_t entryIndex) const {
            return TFacetEntryAcc(Contents().Entries[entryIndex]);
        }
        TConstFacetEntryAcc GetEntry(size_t entryIndex) const {
            return TConstFacetEntryAcc(Contents().Entries[entryIndex]);
        }

        auto Entries() const -> decltype(NDetail::MakeAccContainer<TFacetEntryAcc>(std::declval<DataType&>().Entries)) {
            return NDetail::MakeAccContainer<TFacetEntryAcc>(Contents().Entries);
        }
        auto GetEntries() const -> decltype(NDetail::MakeAccContainer<TConstFacetEntryAcc>(std::declval<DataType&>().Entries)) {
            return NDetail::MakeAccContainer<TConstFacetEntryAcc>(Contents().Entries);
        }
    };

    template <typename DataType>
    class TRequestAccBase
        : public NDetail::TLightAccessor<DataType>
    {
    public:
        using NDetail::TLightAccessor<DataType>::Contents;

        TRequestAccBase() = default;
        TRequestAccBase(const TRequestAcc& other)
            : NDetail::TLightAccessor<DataType>(other.Contents())
        {}
        TRequestAccBase(const TConstRequestAcc& other)
            : NDetail::TLightAccessor<DataType>(other.Contents())
        {}
        TRequestAccBase(DataType& data)
            : NDetail::TLightAccessor<DataType>(data)
        {}
        TRequestAccBase& operator=(const TRequestAccBase&) = default;

        size_t GetNumWords() const {
            return Contents().Words.size();
        }
        ui32 GetAnchorsSrcLength() const {
            return Contents().AnchorsSrcLength;
        }
        TRequestWordAcc Word(size_t wordIndex) const {
            return TRequestWordAcc(Contents().Words[wordIndex]);
        }
        TConstRequestWordAcc GetWord(size_t wordIndex) const {
            return TConstRequestWordAcc(Contents().Words[wordIndex]);
        }

        auto Words() const -> decltype(NDetail::MakeAccContainer<TRequestWordAcc>(std::declval<DataType&>().Words)) {
            return NDetail::MakeAccContainer<TRequestWordAcc>(Contents().Words);
        }
        auto GetWords() const -> decltype(NDetail::MakeAccContainer<TConstRequestWordAcc>(std::declval<DataType&>().Words)) {
            return NDetail::MakeAccContainer<TConstRequestWordAcc>(Contents().Words);
        }

        size_t GetNumMatches() const {
            return Contents().Matches.size();
        }
        TMatchAcc Match(size_t matchIndex) const {
            return TMatchAcc(Contents().Matches[matchIndex]);
        }
        TConstMatchAcc GetMatch(size_t matchIndex) const {
            return TConstMatchAcc(Contents().Matches[matchIndex]);
        }

        auto Matches() const -> decltype(NDetail::MakeAccContainer<TMatchAcc>(std::declval<DataType&>().Matches)) {
            return NDetail::MakeAccContainer<TMatchAcc>(Contents().Matches);
        }
        auto GetMatches() const -> decltype(NDetail::MakeAccContainer<TConstMatchAcc>(std::declval<DataType&>().Matches)) {
            return NDetail::MakeAccContainer<TConstMatchAcc>(Contents().Matches);
        }


        TProxAcc ProxAfter(size_t wordIndex) const {
            return TProxAcc(Contents().Proxes[wordIndex]);
        }
        TConstProxAcc GetProxAfter(size_t wordIndex) const {
            return TConstProxAcc(Contents().Proxes[wordIndex]);
        }

        TFacetsAcc Facets() const {
            return TFacetsAcc(Contents().Facets);
        }
        TConstFacetsAcc GetFacets() const {
            return TConstFacetsAcc(Contents().Facets);
        }

        const TMaybe<TRequestTrCompatibilityInfo>& GetTrCompatibilityInfo() const {
            return Contents().TrCompatibilityInfo;
        }

        void SetTrCompatibilityInfo(const TRequestTrCompatibilityInfo& info) {
            Contents().TrCompatibilityInfo = info;
        }

        void ClearTrCompatibilityInfo() {
            Contents().TrCompatibilityInfo.Clear();
        }

        bool HasExpansionType(EExpansionType type) const {
            for (const auto& facet : GetFacets().GetEntries()) {
                if (facet.GetExpansion() == type) {
                    return true;
                }
            }
            return false;
        }
    };

    namespace NDetail {
        EValidType IsValidFacet(TConstFacetEntryAcc entry);
    } // NDetail
} // NReqBundle
