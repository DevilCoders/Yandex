#pragma once

#include "form_index_assigner.h"
#include "global_options.h"

#include <kernel/reqbundle/block_accessors.h>

#include <util/digest/multi.h>
#include <util/memory/pool.h>

class IFormClassifier;

namespace NReqBundleIteratorImpl {
    struct TFormWithLang {
        TWtringBuf Form;
        ELanguage Lang;
    };
}

template <>
struct THash<NReqBundleIteratorImpl::TFormWithLang> {
    ui64 operator ()(const NReqBundleIteratorImpl::TFormWithLang& x) const {
        return MultiHash(x.Form, x.Lang);
    }
};

template <>
struct TEqualTo<NReqBundleIteratorImpl::TFormWithLang> {
    bool operator ()(
        const NReqBundleIteratorImpl::TFormWithLang& x,
        const NReqBundleIteratorImpl::TFormWithLang& y) const
    {
        return x.Form == y.Form && x.Lang == y.Lang;
    }
};

namespace NReqBundleIteratorImpl {
    struct TLemmBundleData {
        using TForm2LemmId = THashMap<
            TFormWithLang,
            size_t,
            THash<TFormWithLang>,
            TEqualTo<TFormWithLang>,
            TPoolAllocator>;

        using TFormRange = std::pair<ui32, ui32>;

        bool IsStopWord = false;
        bool IsAttribute = false;
        NLP_TYPE NlpType = NLP_END;
        size_t DefaultLemmId = 0;
        TFormRange DefaultFormRange = {0, 0};
        TForm2LemmId Form2LemmId;
        THolder<IFormClassifier> FormClassifier;

        TLemmBundleData(
            NReqBundle::TConstWordAcc word,
            size_t lemmId,
            TFormRange formRange,
            TMemoryPool& pool);
        ~TLemmBundleData() {}

        void AddRichTreeLemma(
            NReqBundle::TConstWordAcc word,
            size_t lemmId,
            TFormRange formRange,
            TMemoryPool& pool);
    };

    struct TWordBundleData {
        using TLemm2Data = THashMap<
            TWtringBuf,
            THolder<TLemmBundleData, TDestructor>,
            THash<TWtringBuf>,
            TEqualTo<TWtringBuf>,
            TPoolAllocator>;

        TFormIndexAssigner Form2Id;
        TLemm2Data Lemm2Data;
        bool AnyWord = false;

        TWordBundleData(
            NReqBundle::TConstWordAcc word,
            TMemoryPool& pool,
            const NReqBundleIterator::TGlobalOptions& globalOptions);
    };

    struct TBlockBundleData {
        using EBlockType = NReqBundle::NDetail::EBlockType;

        size_t NumWords = 0;
        TVector<TWordBundleData, TPoolAllocator> Words;
        ui32 Distance = 0;
        EBlockType Type = EBlockType::Unordered;

        TBlockBundleData(
            NReqBundle::TConstBlockAcc block,
            TMemoryPool& pool,
            const NReqBundleIterator::TGlobalOptions& globalOptions);
    };
} // NReqBundleIteratorImpl
