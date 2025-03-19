#pragma once

#include <util/generic/buffer.h>
#include <util/generic/ptr.h>
#include <util/generic/refcount.h>
#include "indexfile.h"
#include "indexstorageface.h"
#include "indexstorageimpl.h"
#include "indexwriter.h"

namespace NIndexerCore {
    namespace NIndexerDetail {

        //! represents a wrapper of TBuffer for TOutputIndexFileImpl<TStream>
        //! @todo implement specialization of TInputIndexStream<> for TInputMemoryStream:
        //!       template <> class TInputIndexStream<TInputMemoryStream> { .. };
        //!       to eliminate double buffering
        class TInputMemoryStream {
            struct TImpl : public TSimpleRefCount<TImpl> { // TAtomicCounter> {
                const char* Data;
                const size_t Size;
                size_t Pointer;
                TImpl(const char* data, size_t size)
                    : Data(data)
                    , Size(size)
                    , Pointer(0)
                {
                }
                size_t Read(void* buf, size_t len) {
                    if (Pointer + len > Size)
                        len = Size - Pointer;
                    if (len != 0)
                        memcpy(buf, Data + Pointer, len);
                    Pointer += len;
                    return len;
                }
                void Load(void* buf, size_t len) {
                    Read(buf, len);
                }
                int ReadChar() {
                    if (Pointer == Size)
                        return EOF;
                    Y_ASSERT(Pointer < Size);
                    const unsigned char c = Data[Pointer++];
                    return static_cast<int>(c);
                }
                i64 Seek(i64 offset, SeekDir origin) {
                    if (origin == sSet) {
                        if (offset < 0 || (ui64)offset > Size)
                            ythrow yexception() << "invalid offset";
                        Pointer = offset;
                    } else if (origin == sEnd) {
                        if (offset > 0 || offset < -(i64)Size)
                            ythrow yexception() << "invalid offset";
                        Pointer = (i64)Size + offset;
                    } else if (origin == sCur) {
                        Y_ASSERT(!"not implemented");
                    } else
                        Y_ASSERT(!"unexpected origin");
                    return Pointer;
                }
                size_t AvailableBytes() const {
                    return (Size - Pointer);
                }
            };
            TSimpleIntrusivePtr<TImpl> Impl;
        public:
            //! @note in some cases inv-file is not required, so the stream will be closed
            TInputMemoryStream() {
            }
            TInputMemoryStream(const void* data, size_t size)
                : Impl(new TImpl(reinterpret_cast<const char*>(data), size))
            {
            }
            void Close() { // do nothing
                // Impl->Pointer = 0; ?
            }
            bool IsOpen() const {
                return Impl.Get() != nullptr;
            }
            size_t Read(void* buf, size_t len) {
                Y_ASSERT(Impl.Get());
                return Impl->Read(buf, len);
            }
            void Load(void* buf, size_t len) {
                Y_ASSERT(Impl.Get());
                Impl->Load(buf, len);
            }
            int ReadChar() {
                Y_ASSERT(Impl.Get());
                return Impl->ReadChar();
            }
            i64 Seek(i64 offset, SeekDir origin) {
                return Impl->Seek(offset, origin);
            }
        };

        template <>
        class TInputIndexStream<TInputMemoryStream> : public TNonCopyable {
            TInputMemoryStream File;
        public:
            explicit TInputIndexStream(const TInputMemoryStream& file)
                : File(file)
            {
            }
            void Close() {
                File.Close();
            }
            bool IsOpen() const {
                return File.IsOpen();
            }
            bool operator!() const {
                return !IsOpen();
            }
            size_t Read(void* buffer, size_t size) {
                return File.Read(buffer, size);
            }
            int ReadChar() {
                return File.ReadChar();
            }
            size_t ReadString(char* buffer, size_t size) {
                return Read(buffer, size);
            }
        };

        //! represents a wrapper of TBuffer for TOutputIndexFileImpl<TStream>
        //! @todo implement specialization of TOutputIndexStream<> for TOutputMemoryStream:
        //!       template <> class TOutputIndexStream<TOutputMemoryStream> { .. };
        //!       to eliminate double buffering
        class TOutputMemoryStream {
            struct TImpl : public TSimpleRefCount<TImpl> { // TAtomicCounter> {
                TBuffer Buffer; // buffer for reading/writing (only written bytes can be read)
                void Write(const void* buf, size_t len) {
                    Buffer.Append(reinterpret_cast<const char*>(buf), len);
                }
                void WriteChar(char ch) {
                    Buffer.Append(ch);
                }
            };
            TSimpleIntrusivePtr<TImpl> Impl;
        public:
            class TDirectBuffer {
                TImpl* Impl;
                size_t Reserved;
            public:
                explicit TDirectBuffer(const TOutputMemoryStream& owner, size_t reserved)
                    : Impl(owner.Impl.Get())
                    , Reserved(reserved)
                {
                    Y_VERIFY(Impl->Buffer.Avail() >= Reserved, "indexfile library is buggy!");
                    Y_ASSERT(Impl);
                }
                char* GetPointer() {
                    return Impl->Buffer.Pos();
                }
                void ApplyChanges(size_t len) {
                    Y_VERIFY(Reserved >= len, "indexfile library is buggy!");
                    Impl->Buffer.Advance(len);
                }
            };
            TOutputMemoryStream()
                : Impl(new TImpl())
            {
            }
            void Close() { // do nothing
            }
            void Flush() { // do nothing
            }
            bool IsOpen() const {
                return true; // always open
            }
            const TBuffer& GetBuffer() const {
                return Impl->Buffer;
            }
            void SwapBuffer(TBuffer& buf) {
                Impl->Buffer.Swap(buf);
            }
            void Write(const void* buf, size_t len) {
                Impl->Write(buf, len);
            }
            void WriteChar(char val) {
                Impl->WriteChar(val);
            }
            void EnsureBufferSize(size_t minSize) {
                TBuffer& buf = Impl->Buffer;
                if (minSize > buf.Avail()) {
                    const size_t newSize = buf.Size() + minSize;
                    buf.Reserve(newSize);
                }
            }
        };

        template <>
        class TOutputIndexStream<TOutputMemoryStream> : public TNonCopyable {
            TOutputMemoryStream File;
        public:
            typedef TOutputMemoryStream::TDirectBuffer TDirectBuffer;
            explicit TOutputIndexStream(const TOutputMemoryStream& file)
                : File(file)
            {
            }
            void Close() {
                File.Close();
            }
            bool IsOpen() const {
                return File.IsOpen();
            }
            bool operator!() const {
                return !IsOpen();
            }
            void Flush() {
                File.Flush();
            }
            void Write(const void* buffer, size_t size) {
                File.Write(buffer, size);
            }
            void WriteChar(int c) {
                const unsigned char ch = static_cast<unsigned char>(c);
                File.WriteChar(ch);
            }
            void WriteString(const char* str, size_t len) {
                Write(str, len);
            }
            TDirectBuffer GetDirectBuffer(size_t minSize) {
                File.EnsureBufferSize(minSize);
                return TDirectBuffer(File, minSize);
            }
        };

    } // namespace NIndexerDetail

    class TMemoryPortion : public IYndexStorage, private TNonCopyable {
        NIndexerDetail::TOutputMemoryStream KeyStream;
        NIndexerDetail::TOutputMemoryStream InvStream;
        TOutputIndexFileImpl<NIndexerDetail::TOutputMemoryStream> Index;
        typedef TInvKeyWriterImpl<
            TOutputIndexFileImpl<NIndexerDetail::TOutputMemoryStream>, THitWriterImpl<CHitCoder, NIndexerDetail::TOutputMemoryStream>,
            NIndexerDetail::TNoSubIndexWriter, NIndexerDetail::TNoFastAccessTableWriter> TMemoryPortionWriter;
        TMemoryPortionWriter Writer;
        typedef NIndexerDetail::TIndexStorageImpl<TOutputIndexFileImpl<NIndexerDetail::TOutputMemoryStream>, TMemoryPortionWriter> TStorage;
        TStorage Storage;
    public:
        explicit TMemoryPortion(IYndexStorage::FORMAT format, ui32 version = YNDEX_VERSION_CURRENT)
            : Index(KeyStream, InvStream, format, version)
            , Writer(Index, false)
            , Storage(Index, Writer)
        {
        }
        void Flush() {
            Storage.Flush();
            Index.Flush();
        }
        void StorePositions(const char* keyText, SUPERLONG* positions, size_t posCount) override {
            Storage.StorePositions(keyText, positions, posCount);
        }
        void StoreKey(const char* key) {
            Storage.StoreKey(key);
        }
        void StoreHit(SUPERLONG hit) {
            Storage.StoreHit(hit);
        }
        IYndexStorage::FORMAT GetFormat() const {
            return Index.GetFormat();
        }
        const TBuffer& GetKeyBuffer() const {
            return KeyStream.GetBuffer();
        }
        const TBuffer& GetInvBuffer() const {
            return InvStream.GetBuffer();
        }
        void WriteVersion() {
            Y_ASSERT(GetFormat() == IYndexStorage::FINAL_FORMAT);
            WriteVersionData(InvStream, Index.GetVersion());
            WriteIndexStat(InvStream, false, 0, 0, 0);
        }
        void SwapBuffers(TBuffer& keyBuf, TBuffer& invBuf) {
            KeyStream.SwapBuffer(keyBuf);
            InvStream.SwapBuffer(invBuf);
        }
    };

    class TMemoryPortionFactory : public IYndexStorageFactory {
        const IYndexStorage::FORMAT Format;
        const ui32 Version;
    public:
        typedef TVector<const TMemoryPortion*> TPortions;
        //! @param version    used for IYndexStorage::FINAL_FORMAT portions
        explicit TMemoryPortionFactory(IYndexStorage::FORMAT format = IYndexStorage::PORTION_FORMAT, ui32 version = YNDEX_VERSION_CURRENT)
            : Format(format)
            , Version(version)
        {
        }
        ~TMemoryPortionFactory() override {
            ClearPortions();
        }
        void GetStorage(IYndexStorage** storage, IYndexStorage::FORMAT /*type*/) override {
            if (Format == IYndexStorage::FINAL_FORMAT)
                *storage = new TMemoryPortion(Format, Version);
            else
                *storage = new TMemoryPortion(Format, YNDEX_VERSION_CURRENT);
        }
        void ReleaseStorage(IYndexStorage* storage) override {
            TMemoryPortion* portion = dynamic_cast<TMemoryPortion*>(storage);
            if (portion) {
                portion->Flush();
                if (portion->GetFormat() == IYndexStorage::FINAL_FORMAT)
                    portion->WriteVersion();
                Portions.push_back(portion);
            } else if (storage)
                ythrow yexception() << "pointer to unknown object";
        }
        IYndexStorage::FORMAT GetFormat() const {
            return Format;
        }
        const TPortions& GetPortions() const {
            return Portions;
        }
        void ClearPortions() {
            for (size_t i = 0; i < Portions.size(); ++i)
                delete Portions[i];
            Portions.clear();
        }
    private:
        TPortions Portions; //!< released portions
    };

    inline void CopyMemoryPortion(TBuffer& buf, const TMemoryPortion& portion) {
        const TBuffer& keyBuffer = portion.GetKeyBuffer();
        ui32 n = keyBuffer.Size();
        buf.Append(reinterpret_cast<const char*>(&n), sizeof(ui32));
        buf.Append(keyBuffer.Data(), keyBuffer.Size());
        const TBuffer& invBuffer = portion.GetInvBuffer();
        n = invBuffer.Size();
        buf.Append(reinterpret_cast<const char*>(&n), sizeof(ui32));
        buf.Append(invBuffer.Data(), invBuffer.Size());
    }

    inline void CopyMemoryPortions(TBuffer& buf, const TMemoryPortionFactory::TPortions& portions) {
        for (size_t i = 0; i < portions.size(); ++i) {
            CopyMemoryPortion(buf, *portions[i]);
        }
    }

} // namespace NIndexerCore
