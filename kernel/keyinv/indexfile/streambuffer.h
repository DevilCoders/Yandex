#pragma once

#include <util/generic/ptr.h>
#include <util/generic/noncopyable.h>
#include <util/generic/typetraits.h>
#include <util/system/mutex.h>
#include <util/system/guard.h>

#define FS_BLOCK_SIZE 512

namespace NIndexerCore {
    namespace NIndexerDetail {

        //! @note TStream must have the following methods: GetPosition, Read
        template <typename TStream>
        class TInputBuffer : private TNonCopyable {
            TStream* Stream;
            TArrayHolder<unsigned char> Buffer; //!< underlying buffer
            unsigned char* Pointer;
            size_t Size;
            size_t AvailableBytes;
            bool Eof;
        public:
            TInputBuffer()
                : Stream(nullptr)
                , Pointer(nullptr)
                , Size(0)
                , AvailableBytes(0)
                , Eof(false)
            {
            }
            size_t GetSize() const {
                return Size;
            }
            //! @param bufSize    it must not be equal to 0
            void Allocate(TStream& stream, size_t bufSize) {
                Y_ASSERT(!Buffer);
                Y_ASSERT(bufSize);
                Stream = &stream;
                Buffer.Reset(new unsigned char[bufSize]);
                Pointer = Buffer.Get();
                Size = bufSize;
                AvailableBytes = 0;
            }
            void Free() {
                Stream = nullptr;
                Buffer.Reset(nullptr);
                Pointer = nullptr;
                Size = 0;
                AvailableBytes = 0;
            }
            //! @note behavior is similar to fgetc()
            int ReadChar() {
                if (!AvailableBytes) {
                    if (Eof)
                        return EOF;
                    ReadFromStream();
                    if (!AvailableBytes)
                        return EOF;
                }
                Y_ASSERT(Pointer);
                const unsigned char c = *Pointer++;
                --AvailableBytes;
                return static_cast<int>(c);
            }
            //! @note always return required number of bytes if they are available in the stream
            //!       but all data goes through the buffer
            //! @return 0 if there is no data - EOF reached
            size_t Read(void* buffer, size_t size) {
                Y_ASSERT(size <= Size);
                unsigned char* p = static_cast<unsigned char*>(buffer);
                size_t n = size;
                do {
                    if (n <= AvailableBytes) {
                        ReadFromBuffer(p, n);
                        p += n;
                        break;
                    } else if (AvailableBytes) {
                        const size_t m = AvailableBytes;
                        ReadFromBuffer(p, m);
                        p += m;
                        n -= m;
                        AvailableBytes = 0;
                    }
                    if (Eof)
                        break;
                    ReadFromStream();
                } while (AvailableBytes);
                return p - static_cast<unsigned char*>(buffer);
            }
            i64 GetPosition() const {
                Y_ASSERT(Stream);
                return Stream->GetPosition() - AvailableBytes;
            }
            void Clear() {
                AvailableBytes = 0;
            }
        private:
            void ReadFromBuffer(void* buffer, size_t size) {
                Y_ASSERT(Stream);
                Y_ASSERT(size <= AvailableBytes);
                memcpy(buffer, Pointer, size);
                Pointer += size;
                AvailableBytes -= size;
            }
            //! reads data from the stream to the internal buffer
            void ReadFromStream() {
                Y_ASSERT(Stream);
                Y_ASSERT(!AvailableBytes);
                Pointer = Buffer.Get();
                AvailableBytes = Stream->RawRead(Pointer, Size);
                if (AvailableBytes < Size)
                    Eof = true;
            }
        };

        //! @note TStream must have the following methods: GetPosition, Write
        template <typename TStream>
        class TOutputBuffer : private TNonCopyable {
            TStream* Stream;
            TArrayHolder<unsigned char> Buffer; //!< underlying buffer
            unsigned char* Pointer;
            size_t Size;
            size_t AvailableBytes;
        public:
            TMutex* WriteMutex;
        public:
            class TDirectBuffer {
                TOutputBuffer<TStream>* const Owner;
            public:
                explicit TDirectBuffer(TOutputBuffer<TStream>* owner)
                    : Owner(owner)
                {
                    Y_ASSERT(Owner);
                }
                char* GetPointer() const {
                    return reinterpret_cast<char*>(Owner->Pointer);
                }
                void ApplyChanges(size_t len) {
                    Y_ASSERT(Owner->AvailableBytes >= len);
                    Owner->Pointer += len;
                    Owner->AvailableBytes -= len;
                }
            };
            TOutputBuffer()
                : Stream(nullptr)
                , Pointer(nullptr)
                , Size(0)
                , AvailableBytes(0)
                , WriteMutex(nullptr)
            {
            }
            size_t GetSize() const {
                return Size;
            }
            //! @param bufSize    it must not be equal to 0
            void Allocate(TStream& stream, size_t bufSize) {
                Y_ASSERT(!Buffer);
                Y_ASSERT(bufSize >= sizeof(long long));
                Stream = &stream;
                Buffer.Reset(new unsigned char[bufSize]);
                Pointer = Buffer.Get();
                Size = bufSize;
                AvailableBytes = bufSize;
            }
            void Free() {
                Stream = nullptr;
                Buffer.Reset(nullptr);
                Pointer = nullptr;
                Size = 0;
                AvailableBytes = 0;
            }
            //! @note behavior is similar to fpuhc()
            void WriteChar(int c) {
                const unsigned char ch = static_cast<unsigned char>(c);
                WriteInteger(ch);
            }
            template <typename T>
            void WriteInteger(T val) {
                static_assert(std::is_integral<T>::value, "expect std::is_integral<T>::value");
                const size_t n = sizeof(T);
                if (AvailableBytes < n)
                    WriteToStream(Size - AvailableBytes);
                Y_ASSERT(Pointer);
                Y_ASSERT(AvailableBytes >= n);
                *reinterpret_cast<T*>(Pointer) = val;
                Pointer += n;
                AvailableBytes -= n;
            }
            //! @note data with length that is greater than Size are written directly to the stream
            //!       there can be problems on linux if stream was open with directIO == true and
            //!       length is not multiple of 512
            void Write(const void* data, size_t len) {
                if (len > Size) {
                    if (AvailableBytes != Size)
                        WriteToStream(Size - AvailableBytes);
                    Stream->Write(data, len);
                    return;
                }
                const unsigned char* p = static_cast<const unsigned char*>(data);
                size_t n = len;
                for (;;) {
                    if (n <= AvailableBytes) {
                        WriteToBuffer(p, n);
                        break;
                    } else if (AvailableBytes) {
                        const size_t m = AvailableBytes;
                        WriteToBuffer(p, m);
                        p += m;
                        n -= m;
                    }
                    Y_ASSERT(!AvailableBytes);
                    WriteToStream(Size);
                }
            }
            i64 GetPosition() const {
                Y_ASSERT(Stream);
                return Stream->GetPosition() + (Pointer - Buffer.Get());
            }
            void FlushNoLock(bool aligned = false) {
                const size_t len = Pointer - Buffer.Get();
                if (len) {
                    if (aligned) {
                        size_t lenal = len & ~(FS_BLOCK_SIZE - 1);
                        if (lenal)
                            Stream->Write(Buffer.Get(), lenal);
                        Pointer = Buffer.Get();
                        AvailableBytes = Size;
                        if (len > lenal) {
                            ui32 left = len - lenal;
                            memmove(Buffer.Get(), Buffer.Get() + lenal, left);
                            Pointer += left;
                            AvailableBytes -= left;
                        }
                    }
                    else {
                        Stream->ResetDirect();
                        WriteToStream(len);
                    }
                }
            }
            void Flush(bool aligned = false) {
                if (WriteMutex) {
                    TGuard<TMutex> guard(WriteMutex);
                    FlushNoLock(aligned);
                } else
                    FlushNoLock(aligned);
            }
            TDirectBuffer GetDirectBuffer(size_t minSize) {
                if (minSize > AvailableBytes)
                    Flush(true);
                Y_ASSERT(minSize <= AvailableBytes);
                return TDirectBuffer(this);
            }
        private:
            void WriteToStream(size_t len) {
                Y_ASSERT(Stream);
                Y_ASSERT(len && (len == size_t(Pointer - Buffer.Get())));
                Stream->Write(Buffer.Get(), len);
                Pointer = Buffer.Get();
                AvailableBytes = Size;
            }
            void WriteToBuffer(const void* data, size_t len) {
                Y_ASSERT(len <= AvailableBytes);
                memcpy(Pointer, data, len);
                Pointer += len;
                AvailableBytes -= len;
            }
        };

    } // NIndexerDetail
} // NIndexerCore

