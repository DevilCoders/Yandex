#pragma once

#include "enum_map.h"
#include "enum.h"

#include <kernel/country_data/countries.h>

#include <library/cpp/wordpos/wordpos.h>

#include <util/digest/multi.h>
#include <util/generic/strbuf.h>
#include <util/generic/hash.h>

namespace NLingBoost {
    struct TGlobalConstants {
        static constexpr size_t MaxQueryLengthInKvBaseInBytes = 250;
        static constexpr size_t MaxExpansionsLengthInKvBaseInBytes = 250;
    };

    // Useful sets of regions.
    // Should form tree-like hierarchy,
    // i.e. A \cap B should be one of {A, B, empty}.
    struct TRegionClassStruct {
        enum EType {
            Any = 0,
            World = 1,
            Country = 2,
            SmallRegion = 30, // placeholder for all regions
                              // that are not captured by classes above
            NotRegion = 100,
            RegionClassMax
        };
        static const size_t Size = RegionClassMax;
        static TArrayRef<const EType> GetValues() {
            return ::NLingBoost::NDetail::GetStaticRegion<EType, Any, World, Country, SmallRegion, NotRegion>();
        }
    };
    using TRegionClass = TEnumOps<TRegionClassStruct>;
    using ERegionClassType = TRegionClass::EType;

    struct TRegionId;

    ERegionClassType GetRegionClassByRegionId(TRegionId region);
    bool IsRegionClassSubSet(ERegionClassType regionClassX, ERegionClassType regionClassY);
    TRegionId EncodeRegionClass(ERegionClassType regionality);
    ERegionClassType DecodeRegionClass(TRegionId id);

    struct TRegionId {
        static const i64 RegionClassEncodingOffset = -1000;

        i64 Value = 0;

        TRegionId() = default;
        explicit TRegionId(i64 value)
            : Value(value)
        {}

        operator i64 () const {
            return Value;
        }

        bool IsElementOf(ERegionClassType regionClass) const {
            return IsRegionClassSubSet(GetRegionClassByRegionId(*this), regionClass);
        }
        bool IsRegionClass() const {
            return DecodeRegionClass(*this) != TRegionClass::RegionClassMax;
        }
        bool IsRegion() const {
            return Value >= 0;
        }
        bool IsValid() const {
            return IsRegion() || IsRegionClass();
        }

        static TRegionId World() {
            return TRegionId(0);
        }
        static TRegionId Russia() {
            return TRegionId(COUNTRY_RUSSIA.CountryId);
        }
        static TRegionId Turkey() {
            return TRegionId(COUNTRY_TURKEY.CountryId);
        }
        static TRegionId ClassWorld() {
            return EncodeRegionClass(TRegionClass::World);
        }
        static TRegionId ClassCountry() {
            return EncodeRegionClass(TRegionClass::Country);
        }

        bool operator < (const TRegionId& other) const {
            const ERegionClassType thisClass = DecodeRegionClass(*this);
            const ERegionClassType otherClass = DecodeRegionClass(other);

            return (!IsRegionClass() && (other.IsRegionClass() || Value < other.Value))
                || (IsRegionClass() && other.IsRegionClass() && otherClass < thisClass);
        }
    };

    struct TRequestFormStruct {
        enum EType {
            Norm = 0,   // qnorm ~ TFastNormalizer
            Dopp = 1,   // dnorm ~ TDoppelgangersNormalize
            Raw = 2,    // raw user request, no normalization
            Other = 10, // smth else, mostly for experiments
            RequestFormMax
        };
        static const size_t Size = RequestFormMax;
        static TArrayRef<const EType> GetValues() {
            return ::NLingBoost::NDetail::GetStaticRegion<EType, Norm, Dopp, Raw, Other>();
        }
    };
    using TRequestForm = TEnumOps<TRequestFormStruct>;
    using ERequestFormType = TRequestForm::EType;

    struct TExpansionStruct {
        // CAUTION. Constants from this enum
        // are used in proto format as raw integers.
        // Changing value of enum constant will break
        // qbundle format in **production**.
        enum EType {
            OriginalRequest = 0,
            //
            // Main expansion types
            //
            Experiment = 3,
            XfDtShow = 5,
            QueryToSnippet = 6,
            QueryToDoc = 10,
            QueryToKeywords = 15,
            RequestWithRegionName = 16,
            Qfuf = 17,
            QueryToText = 18,
            XfDtShowKnn = 19,
            //
            //Images expansion types
            //
            XfImgClicks = 20,
            ImgClickSim = 21,
            //
            //AdvMachine expansion types
            //
            AdvMachineSelectType123 = 30,
            AdvMachineSelectType26 = 31,
            AdvMachineSelectType98 = 32,
            AdvMachineSelectType57 = 33,
            AdvMachineSelectType101 = 34,
            AdvMachineSelectType89 = 35,
            AdvMachineSelectType96 = 36,
            AdvMachineSelectTypeOther = 37,
            ContextMachinePageTitle = 38,
            ContextMachineUrl = 39,
            //
            //Web expansion types (contd.)
            //
            QueryToTextByXfDtShowKnn = 50,
            RequestWithCountryName = 51,
            RequestWithoutVerbs = 52,
            QfufFilteredByXfOneSe = 53,
            OriginalRequestSynonyms = 54,
            OriginalRequestWordsFilteredByDssmSSHard = 55,
            XfOneSeKnn = 56,
            QueryToTextByXfOneSeKnn = 57,
            RequestMultitokens = 58,
            FioFromOriginalRequest = 59,
            //
            // Geo expansion type
            //
            GeoOriginalRequest = 60,
            GeoWhatOnlyRequest = 61,
            //
            // Web expansions type part 2
            //
            AllFioFromOriginalRequest = 65,
            TelFullAttribute = 66,
            //
            // Market expansion types
            //
            MarketClickSim = 70,
            //
            // Video expansion type
            //
            VideoClickSim = 81,
            //
            // Kinopoisk expansion type
            //
            KinopoiskSuggest = 90,
            //
            // Experimental expansion types
            //
            Experiment0 = 100,
            Experiment1 = 101,
            Experiment2 = 102,
            Experiment3 = 103,
            Experiment4 = 104,
            Experiment5 = 105,
            Experiment6 = 106,
            Experiment7 = 107,
            Experiment8 = 108,
            Experiment9 = 109,
            ExpansionMax
        };
        static const size_t Size = ExpansionMax;
        static TArrayRef<const EType> GetValues() {
            return ::NLingBoost::NDetail::GetStaticRegion<EType, OriginalRequest,
                Experiment, XfDtShow, QueryToSnippet, QueryToDoc, QueryToKeywords,
                RequestWithRegionName, Qfuf, QueryToText, XfDtShowKnn,
                XfImgClicks, ImgClickSim,
                AdvMachineSelectType123, AdvMachineSelectType26,
                AdvMachineSelectType98, AdvMachineSelectType57,
                AdvMachineSelectType101, AdvMachineSelectType89,
                AdvMachineSelectType96, AdvMachineSelectTypeOther,
                ContextMachinePageTitle, ContextMachineUrl,
                QueryToTextByXfDtShowKnn, RequestWithCountryName, RequestWithoutVerbs,
                QfufFilteredByXfOneSe, OriginalRequestSynonyms, OriginalRequestWordsFilteredByDssmSSHard,
                XfOneSeKnn, QueryToTextByXfOneSeKnn, RequestMultitokens, FioFromOriginalRequest,
                GeoOriginalRequest, GeoWhatOnlyRequest,
                AllFioFromOriginalRequest,
                TelFullAttribute,
                MarketClickSim,
                VideoClickSim,
                KinopoiskSuggest,
                Experiment0, Experiment1, Experiment2, Experiment3, Experiment4,
                Experiment5, Experiment6, Experiment7, Experiment8, Experiment9>();
        }

        static TStringBuf GetFullName() {
            return TStringBuf("::NLingBoost::TExpansion");
        }
    };
    using TExpansion = TEnumOps<TExpansionStruct>;
    using TExpansionTypeDescr = TEnumTypeDescr<TExpansionStruct>;
    using EExpansionType = TExpansion::EType;

    struct TMatchStruct {
        // CAUTION. Constants from this enum
        // are used in proto format as raw integers.
        // Changing value of enum constant will break
        // break qbundle format in **production**.
        enum EType {
            OriginalWord = 0,
            Synonym = 1,
            MatchMax
        };
        static const size_t Size = MatchMax;
        static TArrayRef<const EType> GetValues() {
            return ::NLingBoost::NDetail::GetStaticRegion<EType, OriginalWord, Synonym>();
        }
    };
    using TMatch = TEnumOps<TMatchStruct>;
    using EMatchType = TMatch::EType;

    struct TMatchPrecisionStruct {
        enum EType {
            Exact = 0,
            Lemma = 1,
            WeirdForma = 2, // Matched by form that is not explicitly listed in rich tree
            Unknown = 3, // Don't know, e.g. in case of synonym match
            MatchPrecisionMax
        };
        static const size_t Size = MatchPrecisionMax;
        static TArrayRef<const EType> GetValues() {
            return ::NLingBoost::NDetail::GetStaticRegion<EType, Exact, Lemma, WeirdForma, Unknown>();
        }
    };
    using TMatchPrecision = TEnumOps<TMatchPrecisionStruct>;
    using EMatchPrecisionType = TMatchPrecision::EType;

    EMatchPrecisionType GetMatchPrecisionByFormClass(EFormClass formClass);

    struct TConstraintStruct {
        enum EType {
            Must = 0,
            MustNot = 1,
            Quoted = 2,
            ConstraintTypeMax
        };
        static const size_t Size = ConstraintTypeMax;
        static TArrayRef<const EType> GetValues() {
            return ::NLingBoost::NDetail::GetStaticRegion<EType, Must, MustNot, Quoted>();
        }
    };
    using TConstraint = TEnumOps<TConstraintStruct>;
    using EConstraintType = TConstraint::EType;

    struct TWordFreqStruct {
        enum EType {
            Default = 0,
            Url = 1,
            Title = 2,
            Body = 3,
            Links = 4,
            Ann = 5,
            WordFreqMax
        };
        static const size_t Size = WordFreqMax;
        static TArrayRef<const EType> GetValues() {
            return ::NLingBoost::NDetail::GetStaticRegion<EType, Default, Url, Title, Body, Links, Ann>();
        }
    };
    using TWordFreq = TEnumOps<TWordFreqStruct>;
    using EWordFreqType = TWordFreq::EType;

    struct TExpansionTraits {
        bool IsMultiRequest = true;
    };

    namespace NPrivate {

        void FillExpansionTraitsByType(TCompactEnumMap<TExpansion, TExpansionTraits>& table);

        class TExpansionTraitsByType : public TCompactEnumMap<TExpansion, TExpansionTraits> {
        public:
            TExpansionTraitsByType() {
                FillExpansionTraitsByType(*this);
            }
            const TExpansionTraits GetExpansionTraits(EExpansionType type);
        };
    }

    inline const TExpansionTraits GetExpansionTraitsByType(EExpansionType type) {
        static NPrivate::TExpansionTraitsByType expansionTraitsByType;
        return expansionTraitsByType.GetExpansionTraits(type);
    }

    inline EMatchPrecisionType GetMatchPrecisionByFormClass(EFormClass formClass) {
        switch (formClass) {
            case EQUAL_BY_STRING: {
                return TMatchPrecision::Exact;
            }
            case EQUAL_BY_LEMMA: {
                return TMatchPrecision::Lemma;
            }
            case EQUAL_BY_SYNONYM: {
                return TMatchPrecision::Unknown;
            }
            case EQUAL_BY_SYNSET: {
                return TMatchPrecision::Unknown;
            }
            case NUM_FORM_CLASSES: {
                Y_ASSERT(false);
                break;
            }
        }
        return TMatchPrecision::MatchPrecisionMax;
    }

    // Compatibility alias
    inline EMatchPrecisionType FormClassToPrecision(EFormClass formClass) {
        return GetMatchPrecisionByFormClass(formClass);
    }

    inline TRegionId EncodeRegionClass(ERegionClassType regionality) {
        TRegionId res(TRegionId::RegionClassEncodingOffset + static_cast<i64>(regionality));
        Y_ASSERT(GetRegionClassByRegionId(res) == TRegionClass::NotRegion);
        return res;
    }

    inline ERegionClassType DecodeRegionClass(TRegionId id) {
        i64 index = -TRegionId::RegionClassEncodingOffset + id.Value;

        if (Y_LIKELY(TRegionClass::HasIndex(index))) {
            return static_cast<ERegionClassType>(index);
        }
        return TRegionClass::RegionClassMax;
    }

    struct TBlockType {
        static constexpr size_t SkipType = 0;
        static constexpr size_t Layer0Type = 1;
        static constexpr size_t Layer1Type = 2;

        static constexpr inline size_t GetAllLayersMask() {
            return (size_t(1) << Layer0Type) | (size_t(1) << Layer1Type);
        }
        static constexpr inline size_t GetLayer1Mask() {
            return (size_t(1) << Layer1Type);
        }
    };
} // NLingBoost

bool FromString(const TStringBuf& text, NLingBoost::ERegionClassType& value);
bool FromString(const TStringBuf& text, NLingBoost::EExpansionType& value);
bool FromString(const TStringBuf& text, NLingBoost::EMatchType& value);
bool FromString(const TStringBuf& text, NLingBoost::EMatchPrecisionType& value);
bool FromString(const TStringBuf& text, NLingBoost::EWordFreqType& value);

template<>
struct THash<::NLingBoost::TRegionId> {
    ui64 operator() (const ::NLingBoost::TRegionId& x) {
        return MultiHash(x.Value);
    }
};