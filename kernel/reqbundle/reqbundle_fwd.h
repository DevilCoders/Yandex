#pragma once

#include "accessor.h"

#include <kernel/lingboost/constants.h>
#include <kernel/lingboost/freq.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/ptr.h>

namespace NReqBundle {
    using NLingBoost::TRegionClass;
    using NLingBoost::ERegionClassType;

    using NLingBoost::TRegionId;

    using NLingBoost::TExpansion;
    using NLingBoost::EExpansionType;

    using NLingBoost::TMatch;
    using NLingBoost::EMatchType;

    using NLingBoost::EConstraintType;

    using NLingBoost::TWordFreq;
    using NLingBoost::EWordFreqType;
    using NLingBoost::InvalidRevFreq;
    using NLingBoost::IsValidRevFreq;
} // NReqBundle

namespace NReqBundle {
    namespace NDetail {
        struct TFormData;
        struct TLemmaData;
        struct TWordData;
        struct TAttributeData;
        struct TBlockData;
        struct TBinaryBlockData;
    } // NDetail

    template <typename DataType> class TFormAccBase;
    using TFormAcc = TFormAccBase<NDetail::TFormData>;
    using TConstFormAcc = TFormAccBase<const NDetail::TFormData>;

    template <typename DataType> class TLemmaAccBase;
    using TLemmaAcc = TLemmaAccBase<NDetail::TLemmaData>;
    using TConstLemmaAcc = TLemmaAccBase<const NDetail::TLemmaData>;

    template <typename DataType> class TWordAccBase;
    using TWordAcc = TWordAccBase<NDetail::TWordData>;
    using TConstWordAcc = TWordAccBase<const NDetail::TWordData>;

    template <typename DataType> class TAttributeAccBase;
    using TAttributeAcc = TAttributeAccBase<NDetail::TAttributeData>;
    using TConstAttributeAcc = TAttributeAccBase<const NDetail::TAttributeData>;

    template <typename DataType> class TBlockAccBase;
    using TBlockAcc = TBlockAccBase<NDetail::TBlockData>;
    using TConstBlockAcc = TBlockAccBase<const NDetail::TBlockData>;

    template <typename DataType> class TBinaryBlockAccBase;
    using TBinaryBlockAcc = TBinaryBlockAccBase<NDetail::TBinaryBlockData>;
    using TConstBinaryBlockAcc = TBinaryBlockAccBase<const NDetail::TBinaryBlockData>;

    class TBlock;
    using TBlockPtr = TIntrusivePtr<TBlock>;

    class TBinaryBlock;
    using TBinaryBlockPtr = TIntrusivePtr<TBinaryBlock>;

    namespace NDetail {
        struct TSequenceElemData;
        struct TSequenceData;
    } // NDetail

    template <typename DataType> class TSequenceElemAccBase;
    using TSequenceElemAcc = TSequenceElemAccBase<NDetail::TSequenceElemData>;
    using TConstSequenceElemAcc = TSequenceElemAccBase<const NDetail::TSequenceElemData>;

    template <typename DataType> class TSequenceAccBase;
    using TSequenceAcc = TSequenceAccBase<NDetail::TSequenceData>;
    using TConstSequenceAcc = TSequenceAccBase<const NDetail::TSequenceData>;

    class TSequence;
    using TSequencePtr = TIntrusivePtr<TSequence>;

    namespace NDetail {
        struct TProxData;
        struct TRequestWordData;
        struct TMatchData;
        struct TConstraintData;
        struct TFacetEntryData;
        struct TFacetsData;
        struct TRequestData;
    } // NDetail

    template <typename DataType> class TProxAccBase;
    using TProxAcc = TProxAccBase<NDetail::TProxData>;
    using TConstProxAcc = TProxAccBase<const NDetail::TProxData>;

    template <typename DataType> class TRequestWordAccBase;
    using TRequestWordAcc = TRequestWordAccBase<NDetail::TRequestWordData>;
    using TConstRequestWordAcc = TRequestWordAccBase<const NDetail::TRequestWordData>;

    template <typename DataType> class TMatchAccBase;
    using TMatchAcc = TMatchAccBase<NDetail::TMatchData>;
    using TConstMatchAcc = TMatchAccBase<const NDetail::TMatchData>;

    template <typename DataType> class TFacetEntryAccBase;
    using TFacetEntryAcc = TFacetEntryAccBase<NDetail::TFacetEntryData>;
    using TConstFacetEntryAcc = TFacetEntryAccBase<const NDetail::TFacetEntryData>;

    template <typename DataType> class TFacetsAccBase;
    using TFacetsAcc = TFacetsAccBase<NDetail::TFacetsData>;
    using TConstFacetsAcc = TFacetsAccBase<const NDetail::TFacetsData>;

    template <typename DataType> class TRequestAccBase;
    using TRequestAcc = TRequestAccBase<NDetail::TRequestData>;
    using TConstRequestAcc = TRequestAccBase<const NDetail::TRequestData>;

    template <typename DataType> class TConstraintAccBase;
    using TConstraintAcc = TConstraintAccBase<NDetail::TConstraintData>;
    using TConstConstraintAcc = TConstraintAccBase<const NDetail::TConstraintData>;

    class TRequest;
    using TRequestPtr = TIntrusivePtr<TRequest>;

    class TContraint;
    using TContraintPtr = TIntrusivePtr<TContraint>;

    namespace NDetail {
        struct TReqBundleData;
    } // NDetail

    template <typename DataType> class TReqBundleAccBase;
    using TReqBundleAcc = TReqBundleAccBase<NDetail::TReqBundleData>;
    using TConstReqBundleAcc = TReqBundleAccBase<const NDetail::TReqBundleData>;

    class TReqBundle;
    using TReqBundlePtr = TIntrusivePtr<TReqBundle>;

    namespace NSer {
        class TSerializer;
        class TDeserializer;
    } // NSer

    class TAllRestrictOptions;
    class TReqBundleSubset;
    class TReqBundleMerger;

    namespace NDetail {
        enum EValidType {
            VT_FALSE = 0,
            VT_UNKNOWN = 1, // unable to check, bool(VT_UNKNOWN) = True
            VT_TRUE = 2
        };

        inline EValidType operator && (EValidType x, EValidType y) {
            return static_cast<EValidType>(::Min(int(x), int(y)));
        }

        struct TValidConstraints {
            bool NeedNonEmpty = false;
            bool NeedOriginal = true;
            bool NeedBlocks = false;
            bool NeedBinaries = false;
            bool IgnoreElems = false;
        };
    } // NDetail
} // NReqBundle

using ::NReqBundle::TReqBundle;
using ::NReqBundle::TReqBundlePtr;

using TReqBundleSerializer = ::NReqBundle::NSer::TSerializer;
using TReqBundleDeserializer = ::NReqBundle::NSer::TDeserializer;

