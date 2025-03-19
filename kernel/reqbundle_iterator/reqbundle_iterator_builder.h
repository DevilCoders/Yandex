#pragma once

#include "index_proc.h"
#include "iterator_impl.h"
#include "reqbundle_iterator.h"
#include "saveload.h"

#include <kernel/reqbundle_iterator/proto/reqbundle_iterator.pb.h>

#include <ysite/yandex/posfilter/filter_tree.h>

namespace NReqBundleIterator {
    class IRBIndexIteratorLoader {
    public:
        virtual ~IRBIndexIteratorLoader() = default;

        virtual NImpl::TIteratorPtr LoadIndexIterator(IInputStream* rh, size_t blockId, TMemoryPool& iteratorsMemory) = 0;
    };

    // Depends only on the global part of the index
    class IRBIteratorBuilderGlobal: public IRBIndexIteratorLoader {
    public:
        IRBIteratorBuilderGlobal(bool needIteratorSorting, bool utf8IndexKeys)
            : NeedIteratorSorting_(needIteratorSorting)
            , Utf8IndexKeys_(utf8IndexKeys)
            , KeyPrefixes({0})
        {
        }

        virtual ~IRBIteratorBuilderGlobal() = default;

        virtual NImpl::IIndexAccessor& GetIndexAccessor() = 0;

        // TODO: move this stuff into some Settings-like class
        bool NeedIteratorSorting() const {
            return NeedIteratorSorting_;
        }

        bool Utf8IndexKeys() const {
            return Utf8IndexKeys_;
        }

        const TVector<ui64>& GetKeyPrefixes() const {
            return KeyPrefixes;
        }

        void SetKeyPrefixes(TVector<ui64> keyPrefixes) {
            KeyPrefixes = std::move(keyPrefixes);
        }

    private:
        bool NeedIteratorSorting_;
        bool Utf8IndexKeys_;
        TVector<ui64> KeyPrefixes;

    };

    class IRBSharedDataFactory {
    public:
        virtual ~IRBSharedDataFactory() = default;

        virtual THolder<NImpl::TSharedIteratorData> MakeSharedData() = 0;
    };


    // TODO: merge with IteratorLoader or get rid of one of them completely
    class IRBIteratorDeserializer {
    public:
        virtual ~IRBIteratorDeserializer() = default;

        virtual NImpl::TIteratorPtr DeserializeFromProto(const NReqBundleIteratorProto::TBlockIterator& proto, TMemoryPool& iteratorsMemory, const ui32 offset = 0) = 0;
    };

    class IRBIteratorBuilder
        : public IRBIteratorBuilderGlobal
        , public IRBSharedDataFactory
    {
    public:
        IRBIteratorBuilder(bool needIteratorSorting, bool utf8IndexKeys)
            : IRBIteratorBuilderGlobal(needIteratorSorting, utf8IndexKeys)
        {
        }

        virtual ~IRBIteratorBuilder() = default;
    };
} // NReqBundleIterator
