#pragma once

#include "data_accessor.h"
#include "position.h"
#include <util/memory/blob.h>

namespace NRTYArchive {

    class IClosable {
    public:
        IClosable(bool closed = false)
            : Closed(closed) {}

        virtual ~IClosable() {}

        void Close() {
            DoClose();
            Closed = true;
        }

        bool IsClosed() const {
            return Closed;
        }

    private:
        virtual void DoClose() = 0;

        bool Closed = false;
    };


    class IArchivePart: public TAtomicRefCount<IArchivePart>, public IClosable {
    public:
        typedef TIntrusivePtr<IArchivePart> TPtr;
        typedef IDataAccessor::TOffset TOffset;
        typedef IDataAccessor::TSize TSize;

        static constexpr TOffset InvalidOffset = ::NRTYArchive::TPosition::InvalidOffset;

        enum TType {
            RAW,
            COMPRESSED,
            DATALESS,
            COMPRESSED_EXT
        };

        class IIterator: public TAtomicRefCount<IIterator> {
        public:
            typedef TIntrusivePtr<IIterator> TPtr;

            virtual ~IIterator() {}
            virtual TBlob GetDocument() const = 0;
            virtual bool SkipTo(TOffset offset) = 0;
            virtual IDataAccessor::TOffset SkipNext() = 0;
            virtual bool CheckOffsetNext(TOffset offset) = 0;
        };

        struct TConstructContext {
            struct TCompressionExtParams {
                TString CodecName = "lz4";
                ui32 BlockSize = 32768;
                ui32 LearnSize = 0;  // only for trainable codecs - zstd, for example

                TString ToString() const;
            };

            struct TCompressionParams {
                enum TAlgorithm { LZ4, LZMA };

                static const ui32 DEFAULT_LEVEL = -1;

                TAlgorithm Algorithm = LZ4;
                ui32 Level = DEFAULT_LEVEL;

                TCompressionExtParams ExtParams;

                TString ToString() const;
            };

            static const TSize NOT_LIMITED_PART;

            TConstructContext(TType type, IDataAccessor::TType daType, TSize sizeLimit = NOT_LIMITED_PART, bool preallocationFlag = false,
                bool withoutSizes = false, const TCompressionParams& compression = Default<TCompressionParams>(), IDataAccessor::TType writeDAType = IDataAccessor::DIRECT_FILE,
                ui32 writeSpeed = 0);

            TConstructContext& SetAccessorType(IDataAccessor::TType daType);
            TConstructContext& SetWithoutSizesFlag(bool flag);
            TConstructContext& SetSizeLimit(TSize sizeLimit);
            TConstructContext& SetMapHeader(bool flag);

            TConstructContext GetWritableContext() const {
                TConstructContext c(*this);
                c.DataAccessorContext.WriteSpeedLimit = WriteSpeedBytes;
                if (SizeLimit != NOT_LIMITED_PART) {
                    c.DataAccessorContext.SizeLimit = SizeLimit;
                }
                c.DataAccessorContext.PreallocationFlag = PreallocationFlag;
                
                return c.SetAccessorType(WriteDAType);
            }

            TString ToString() const;

            TType Type;
            TSize SizeLimit;
            bool PreallocationFlag;
            bool WithoutSizes;
            bool MapHeader;
            ui32 WriteSpeedBytes;
            IDataAccessor::TConstructContext DataAccessorContext;
            TCompressionParams Compression;
            IDataAccessor::TType WriteDAType;
        };

        class IPolicy {
        public:
            virtual ~IPolicy() {}

            virtual TConstructContext GetContext() const = 0;
            virtual bool IsWritable() const = 0;
        };

        typedef NObjectFactory::TParametrizedObjectFactory<IArchivePart, TType, TFsPath, const IPolicy&> TFactory;

    public:
        IArchivePart(bool closed = false)
            : IClosable(closed) {}
        virtual ~IArchivePart() {}
        virtual TBlob GetDocument(TOffset offset) const = 0;
        virtual bool HardLinkOrCopyTo(const TFsPath& path) const = 0;
        virtual IIterator::TPtr CreateIterator() const = 0;
        virtual void Drop() = 0;
        virtual const TFsPath& GetPath() const = 0;
        virtual ui64 GetSizeInBytes() const = 0;

        // returns TPosition::InvalidOffset if the document isn't written
        [[nodiscard]] virtual TOffset TryPutDocument(const TBlob& document) = 0;

        virtual bool IsFull() const = 0;
        virtual TOffset GetWritePosition() const = 0;
    };
}
