#pragma once

#include "index_offroad_accessor.h"
#include "iterator_offroad_impl.h"
#include "old_packed_key_converter.h"
#include "reqbundle_iterator_builder.h"
#include "reqbundle_iterator_offroad_impl.h"
#include "iterator_and_impl.h"
#include "iterator_ordered_and_impl.h"
#include "iterator_impl.h"

#include <kernel/sent_lens/sent_lens.h>

#include <util/memory/pool.h>
#include <util/generic/size_literals.h>

using namespace NReqBundleIteratorImpl;

template<class TOffroadSearcher>
class TReqBundleOffroadSharedDataFactory final: public NReqBundleIterator::IRBSharedDataFactory {
    using IOffroadIterator = typename TOffroadSearcher::IIterator;
public:
    // Keeps non-owning references to the passed in objects
    TReqBundleOffroadSharedDataFactory(
        TOffroadSearcher& searcher,
        const ISentenceLengthsLenReader* sentReader = nullptr)
        : OffroadSearcher_(&searcher)
        , SentenceLengthsReader_(sentReader)
    {
    }

    THolder<TSharedIteratorData> MakeSharedData() override {
        THolder<IOffroadIterator> iter = OffroadSearcher_->MakeIterator();
        return MakeHolder<TOffroadSharedIteratorData<TOffroadSearcher>>(*OffroadSearcher_, std::move(iter), SentenceLengthsReader_);
    }

private:
    TOffroadSearcher* OffroadSearcher_ = nullptr;
    const ISentenceLengthsLenReader* SentenceLengthsReader_ = nullptr;
};

template<class TOffroadSearcher>
class TReqBundleOffroadIndexIteratorLoader final: public NReqBundleIterator::IRBIndexIteratorLoader {
public:
    TIteratorPtr LoadIndexIterator(IInputStream* rh, size_t blockId, TMemoryPool& iteratorsMemory) override {
        ui8 magicNumber = -1;
        LoadPodType(rh, magicNumber);
        Y_ASSERT(magicNumber == OffroadIteratorMagicNumber);
        EIteratorType iteratorType;
        LoadPodType(rh, iteratorType);
        return MakeIteratorOffroad<TOffroadSearcher>(iteratorType, blockId, iteratorsMemory, rh);
    }
};

template<class TOffroadSearcher, class TTermKeyConverter = TTermKeyToOldPackedKeyConverter, bool Utf8IndexKeysParam = false>
class TReqBundleOffroadIteratorBuilderGlobal final: public NReqBundleIterator::IRBIteratorBuilderGlobal {
    using IOffroadIterator = typename TOffroadSearcher::IIterator;
public:
    // Keeps non-owning references to the passed in objects
    TReqBundleOffroadIteratorBuilderGlobal(TOffroadSearcher& searcher)
        : IRBIteratorBuilderGlobal(true, Utf8IndexKeysParam)
        , OffroadIterator_(searcher.MakeIterator())
        , Pool_(64_KB)
        , IndexAccessor_(searcher, *OffroadIterator_, Pool_)
    {
    }

    IIndexAccessor& GetIndexAccessor() override {
        return IndexAccessor_;
    }

    TIteratorPtr LoadIndexIterator(IInputStream* rh, size_t blockId, TMemoryPool& iteratorsMemory) override {
        return IndexIteratorLoader_.LoadIndexIterator(rh, blockId, iteratorsMemory);
    }

private:
    THolder<IOffroadIterator> OffroadIterator_;
    TMemoryPool Pool_;
    TOffroadIndexAccessor<TOffroadSearcher, TTermKeyConverter> IndexAccessor_;
    TReqBundleOffroadIndexIteratorLoader<TOffroadSearcher> IndexIteratorLoader_;
};

template<class TOffroadSearcher, class TTermKeyConverter = TTermKeyToOldPackedKeyConverter, bool Utf8IndexKeysParam = false>
class TReqBundleOffroadIteratorBuilder : public IReqBundleIteratorBuilder {
public:
    using TOffroadIterator = typename TOffroadSearcher::IIterator;

    TReqBundleOffroadIteratorBuilder(
        TOffroadSearcher& searcher,
        const ISentenceLengthsLenReader* sentReader = nullptr)
        : IReqBundleIteratorBuilder(true, Utf8IndexKeysParam)
        , Builder_(searcher)
        , SharedDataFactory_(searcher, sentReader)
    {
    }

    THolder<TSharedIteratorData> MakeSharedData() override {
        return SharedDataFactory_.MakeSharedData();
    }

    IIndexAccessor& GetIndexAccessor() override {
        return Builder_.GetIndexAccessor();
    }

    TIteratorPtr LoadIndexIterator(IInputStream* rh, size_t blockId, TMemoryPool& iteratorsMemory) override {
        return Builder_.LoadIndexIterator(rh, blockId, iteratorsMemory);
    }

private:
    TReqBundleOffroadIteratorBuilderGlobal<TOffroadSearcher, TTermKeyConverter> Builder_;
    TReqBundleOffroadSharedDataFactory<TOffroadSearcher> SharedDataFactory_;
};

template <typename TOffroadSearcher>
class TReqBundleOffroadIteratorDeserializer: public NReqBundleIterator::IRBIteratorDeserializer {
public:
    TIteratorPtr DeserializeFromProto(const NReqBundleIteratorProto::TBlockIterator& proto, TMemoryPool& iteratorsMemory, const ui32 offset = 0) override {
        ui32 blockId = proto.GetBlockId() + offset;
        TIteratorPtr result;
        switch (proto.GetVariantCase()) {
            case NReqBundleIteratorProto::TBlockIterator::kIndexIterator:
                result = FromProto(proto.GetIndexIterator(), blockId, iteratorsMemory);
                break;
            case NReqBundleIteratorProto::TBlockIterator::kAndIterator:
                result = FromProto(proto.GetAndIterator(), blockId, iteratorsMemory);
                break;
            case NReqBundleIteratorProto::TBlockIterator::kOrderedAndIterator:
                result = FromProto(proto.GetOrderedAndIterator(), blockId, iteratorsMemory);
                break;
            default:
                Y_ENSURE(false, "Unknown iterator type: " << static_cast<int>(proto.GetVariantCase()));
        }

        result->BlockType = proto.GetBlockType();
        return result;
    }
private:
    NDoom::TRangedKeyId FromProto(const NReqBundleIteratorProto::TOffroadRangedKeyId& id) const {
        return { id.GetId(), id.GetRange() };
    }

    TIteratorPtr FromProto(const NReqBundleIteratorProto::TIndexIterator& proto, ui32 blockId, TMemoryPool& iteratorsMemory) const {
        Y_ENSURE(proto.HasOffroadIterator());
        TIteratorOffroadPtr offroadIterator = MakeIteratorOffroad<TOffroadSearcher>(
            static_cast<EIteratorType>(proto.GetIteratorType()),
            blockId,
            iteratorsMemory,
            FromProto(proto.GetOffroadIterator().GetOffroadTermId()));
        TPositionTemplates& templates = offroadIterator->GetTemplatesToFill();
        templates.resize(proto.GetOffroadIterator().GetTemplates().GetRawPositions().size());
        for (size_t i : xrange(proto.GetOffroadIterator().GetTemplates().GetRawPositions().size())) {
            templates[i] = proto.GetOffroadIterator().GetTemplates().GetRawPositions(i);
        }
        return offroadIterator;
    }

    TIteratorPtr FromProto(const NReqBundleIteratorProto::TAndIterator& proto, ui32 blockId, TMemoryPool& iteratorsMemory) {
        const ui32 numWords = proto.GetNumWords();
        return MakeIteratorAnd(numWords, blockId, iteratorsMemory, proto, *this);
    }

    TIteratorPtr FromProto(const NReqBundleIteratorProto::TOrderedAndIterator& proto, ui32 blockId, TMemoryPool& iteratorsMemory) {
        const ui32 numWords = proto.GetNumWords();
        const ui64 anyWordsMask = proto.GetAnyWordsMask();
        return MakeIteratorOrderedAnd(numWords, blockId, iteratorsMemory, anyWordsMask, proto, *this);
    }

};
