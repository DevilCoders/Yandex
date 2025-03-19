#pragma once

#include <library/cpp/object_factory/object_factory.h>
#include <util/folder/path.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/generic/maybe.h>

namespace NRTYArchive {

    class IDataAccessor : public TAtomicRefCount<IDataAccessor> {
    public:
        enum TType {
            DIRECT_FILE,
            FILE,
            MEMORY_MAP,
            MEMORY,
            MEMORY_FROM_FILE,
            MEMORY_LOCKED_MAP,
            YT_STREAM, // user is expected to link with saas/library/yt/data_accessor
            MEMORY_PRECHARGED_MAP, // use when index fits into the RAM most of the time but cannot be locked
            MEMORY_RANDOM_ACCESS_MAP // use with care: prefetching is completely disabled
        };

        struct TConstructContext {
            TConstructContext(TType type)
                : Type(type)
                , WriteSpeedLimit(0)
                , PreallocationFlag(false)
            {}
            TType Type;
            ui64 WriteSpeedLimit;
            TMaybe<ui64> SizeLimit;
            bool PreallocationFlag;
        };
        typedef ui64 TOffset;
        typedef ui64 TSize;
        typedef TIntrusivePtr<IDataAccessor> TPtr;
        virtual ~IDataAccessor() {}
        typedef NObjectFactory::TParametrizedObjectFactory<IDataAccessor, TType, TFsPath, TConstructContext, bool> TFactory;

        virtual TSize GetSize() const = 0;
        virtual TBlob Read(TSize size, TOffset offset) = 0;
        virtual bool IsBuffered() const = 0;

        static TPtr Create(const TFsPath& path, const TConstructContext& context);
    };

    class IWritableDataAccessor : public IDataAccessor {
    public:
        typedef TIntrusivePtr<IWritableDataAccessor> TPtr;
        typedef NObjectFactory::TParametrizedObjectFactory<IWritableDataAccessor, TType, TFsPath, TConstructContext, bool> TFactory;
        static TPtr Create(const TFsPath& path, const TConstructContext& context);
        virtual TOffset Write(const void* buffer, TSize size) = 0;
        virtual void Flush() = 0;
    };

}
