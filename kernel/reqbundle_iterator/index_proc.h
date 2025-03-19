#pragma once

#include "iterator_impl.h"
#include "form_index_assigner.h"
#include "bundle_proc.h"

#include <ysite/yandex/posfilter/filter_tree.h>
#include <ysite/yandex/posfilter/leaf.h>
#include <ysite/yandex/posfilter/leafhits.h>
#include <ysite/yandex/posfilter/formfilter.h>

#include <util/memory/pool.h>
#include <util/generic/map.h>
#include <util/generic/deque.h>

namespace NReqBundleIteratorImpl {
    struct TLemmIndexData;
    class TWordIndexData;
    struct TAttributeIndexData;
    class TBlockIndexData;
} // NReqBundleIteratorImpl

namespace NReqBundleIteratorImpl {
    struct TFormInfo {
        ui32 Extra = 0; // Lang, Flags
        TWtringBuf Form;

        bool operator==(const TFormInfo&) const = default;
    };

    struct TFormInfoHash {
        size_t operator()(const TFormInfo& formId) const {
            return MultiHash(formId.Extra, formId.Form);
        }
    };

    struct TIdsPair {
        ui16 LowLevelId = 0;
        ui16 RichTreeId = 0;
    };

    using TYndexFormsCatalog = THashMap<
        TFormInfo,
        TIdsPair,
        TFormInfoHash,
        TEqualTo<TFormInfo>,
        TPoolAllocator>;

    class ILemmHits {
    public:
        virtual ~ILemmHits() {}

        virtual void MakeIterators(
            TIteratorPtrs& target,
            size_t blockId,
            TMemoryPool& iteratorsMemory,
            const TLemmIndexData& lemmData) const = 0;
    };

    class IIndexAccessor {
    public:
        virtual ~IIndexAccessor() {}

        virtual void PrepareHits(
            TStringBuf lemmKey,
            TArrayRef<TLemmIndexData*> lemmData,
            const ui64 keyPrefix,
            const NReqBundleIterator::TRBIteratorOptions& options) = 0;
    };


    struct TIndexLookupKey {
        TStringBuf Key;
        ui64 KeyPrefix = 0;

        bool operator<(const TIndexLookupKey& other) const {
            return std::tie(Key, KeyPrefix) < std::tie(other.Key, other.KeyPrefix);
        }
    };

    struct TLemmIndexData;
    using TIndexLookupMapping = TMap<TIndexLookupKey, TVector<TLemmIndexData*>>; // TMap because we want to iterate it in the sorted order


    struct TLemmIndexData
        : public IYndexFormNotification
    {
        // As opposed to the plain THashMap::const_iterator, this pointer is not invalidated on insertions
        using TYndexCatalogRef = const TYndexFormsCatalog::value_type*;

        TMemoryPool* FormsPool = nullptr;

        THolder<ILemmHits> Hits;

        // Yndex
        TDeque<TYndexCatalogRef, TPoolAllocator> Forms;
        const TFormIndexAssigner* RichTreeFormsCatalog = nullptr;
        TYndexFormsCatalog* YndexFormsCatalog = nullptr;
        const TLemmBundleData* BundleData = nullptr;

        TLemmIndexData(
            TMemoryPool& pool,
            const TWordBundleData* bundleWord,
            TYndexFormsCatalog* forms,
            const TLemmBundleData* bundleData)
            : FormsPool(&pool)
            , Forms(&pool)
            , RichTreeFormsCatalog(&bundleWord->Form2Id)
            , YndexFormsCatalog(forms)
            , BundleData(bundleData)
        {
        }
        virtual ~TLemmIndexData() {}

        TLemmIndexData(TLemmIndexData&&) = default;

        template <class WordFormFlags>
        void PrepareTemplatesForKishka(
            TPositionTemplates& templates,
            const WordFormFlags& flags,
            size_t& formDataPtr) const;

        void OnFormAccepted(
            TWtringBuf yndexForm,
            ui8 yndexFormFlags,
            ELanguage yndexFormLang,
            TWtringBuf richTreeForm) override;

        void MakeIterators(
            TIteratorPtrs& target,
            size_t blockId,
            TMemoryPool& iteratorsMemory);
    };

    class TWordIndexData {
    public:
        TVector<TLemmIndexData, TPoolAllocator> Lemmas;
        size_t NumLemmas = 0;
        TYndexFormsCatalog YndexFormsCatalog;

    public:
        TWordIndexData(TMemoryPool& pool)
            : Lemmas(&pool)
            , YndexFormsCatalog(&pool)
        {
        }

        void CollectLemmas(
            const TWordBundleData& commonData,
            TMemoryPool& pool,
            bool utf8IndexKeys,
            const TVector<ui64>& keyPrefixes,
            TIndexLookupMapping& allKeys);

        void MakeIterators(
            TIteratorPtrs& target,
            TVector<ui16>* richTreeFormIds,
            size_t blockId,
            TMemoryPool& iteratorsMemory);
    };

    class TBlockIndexData {
    public:
        TVector<TWordIndexData, TPoolAllocator> Words;
        size_t NumWords = 0;
        bool NeedFetch = true;
        bool NeedSerialize = false;

    public:
        TBlockIndexData(TMemoryPool& pool)
            : Words(&pool)
        {}

        void CollectIndexKeys(
            const TBlockBundleData& commonData,
            TMemoryPool& pool,
            bool utf8IndexKeys,
            const TVector<ui64>& keyPrefixes,
            TIndexLookupMapping& allKeys);
    };


    template <class WordFormFlags>
    void TLemmIndexData::PrepareTemplatesForKishka(
        TPositionTemplates& templates,
        const WordFormFlags& flags,
        size_t& formDataPtr) const
    {
        templates.resize(flags.GetFormsCount());
        for (size_t j = 0; j < templates.size(); j++) {
            if (flags.GetFormClass(j) == NUM_FORM_CLASSES) {
                templates[j].SetInvalid();
            } else {
                Y_ASSERT(formDataPtr < Forms.size());
                const TFormInfo& formInfo = Forms[formDataPtr]->first;
                const TIdsPair& idsPair = Forms[formDataPtr]->second;
                size_t lemmId = BundleData->DefaultLemmId;
                if (!BundleData->Form2LemmId.empty()) {
                    auto* lemmIdPtr = BundleData->Form2LemmId.FindPtr(
                        TFormWithLang{
                            formInfo.Form,
                            static_cast<ELanguage>(formInfo.Extra >> 16)});

                    if (lemmIdPtr) {
                        lemmId = *lemmIdPtr;
                    }
                }
                templates[j].Clear();
                templates[j].TMatch::SetClean(flags.GetFormClass(j));
                templates[j].TLowLevelFormId::SetClean(idsPair.LowLevelId);
                templates[j].TLemmId::SetClean(lemmId);
                ++formDataPtr;
            }
        }
    }
} // NReqBundleIteratorImpl
