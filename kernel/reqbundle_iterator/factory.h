#pragma once

#include "reqbundle_iterator_fwd.h"
#include "global_options.h"

#include <kernel/reqbundle/reqbundle_accessors.h> // TConstReqBundleAcc
#include <kernel/sent_lens/sent_lens.h> // TSentenceLengthsReader
#include <library/cpp/langmask/langmask.h> // TLangMask

#include <util/memory/pool.h>
#include <util/generic/ptr.h>
#include <util/generic/noncopyable.h>

namespace NReqBundleIterator {
    struct TRBIteratorOptions {
        ui32 TypesMask = ~ui32(0); // deprecated and ignored
        bool EnableOrderedAnd = false;
        ui32 IteratorsFetchLimit = 1000;
        ui32 IteratorsFirstStageFetchLimit = Max<ui32>();
        ui32 FormsPerLemmaLimit = Max<ui32>();
        bool SkipEmptyForms = false;

        TRBIteratorOptions() = default;

        TRBIteratorOptions(ui32 typesMask)
            : TypesMask(typesMask)
        {
        }
    };

    struct TRBIteratorOpeningInfo {
        size_t NumBlocks = 0;
        size_t NumBlocksCached = 0;
    };

    class TRBIteratorsFactory
        : public TNonCopyable
    {
    private:
        class TImpl;
        THolder<TImpl> Impl;

    public:
        TRBIteratorsFactory(
            TConstReqBundleAcc reqBundle,
            const TGlobalOptions& globalOptions = {},
            TMemoryPool* pool = nullptr);

        ~TRBIteratorsFactory();

        // should be set before OpenIterator(); default value is DefaultBlockType
        TBlockType& BlockType(size_t blockIdx);

        TRBIteratorPtr OpenIterator(
            IRBIteratorBuilder& builder,
            IRBIteratorsHasher* hasher = nullptr,
            const TRBIteratorOptions& options = {});

        /* The following two methods implement "two phase" iterator opening:
         * 1) First we perform preliminary work creating some intermediate representation of the future iterator.
         *    This representation can be sent over the network to another machine for further processing.
         * 2) Given the blob obtained from the previous step we open the real usable iterator.
         *
         * Rationale: the index itself is split into two independent parts, both of which are needed to get the
         * full working iterator. We need a way to do it even when different parts of the index are separated by
         * the network.
         */

        /* First phase. "Pre opens" the iterator returning the serialized representation of
         * a partially open iterator.
         */
        TString PreOpenIterator(
            IRBIteratorBuilderGlobal& builder,
            IRBIteratorsHasher* hasher = nullptr,
            const TRBIteratorOptions& options = {},
            TRBIteratorOpeningInfo* info = nullptr);

        /* Finalizes the process and returns a fully functional iterator
         */
        static TRBIteratorPtr OpenIterator(
            TStringBuf intermediateRepresentation,
            IRBSharedDataFactory& sharedDataFactory,
            IRBIteratorDeserializer& deserializer,
            const TRBIteratorOptions& options = {});

        /* Creates template to be used with DeserializeAndAppend
         */
        static TRBIteratorPtr OpenIterator(
            IRBSharedDataFactory& sharedDataFactory,
            const TRBIteratorOptions& options = {});

        /* Appends serialized data to existing iterator
         */
        static void DeserializeAndAppend(
            TStringBuf intermediateRepresentation,
            IRBIteratorDeserializer& deserializer,
            TRBIteratorPtr& rbIteratorPtr);

        /* Does post-append routines (sorts iterators, initializes InternalHeap) and returns true if iterators list isn't empty
         */
        static bool PostAppendProcess(
            TRBIteratorPtr& rbIteratorPtr,
            const TRBIteratorOptions& options = {},
            TRBIteratorsFactory* blockTypes = nullptr);
    };
} // NReqBundleIterator
