#pragma once

#include <util/generic/ptr.h>
#include <util/generic/noncopyable.h>
#include <util/system/filemap.h>
#include <util/system/file.h>

#include <kernel/keyinv/hitlist/longs.h>

#include "streambuffer.h"

namespace NIndexerCore {
    namespace NIndexerDetail {

        template <typename TStream>
        struct TStreamBufferTraits {
            static ui8 get_8(NIndexerCore::NIndexerDetail::TInputBuffer<TStream>*) { Y_ASSERT(false); return 0; } // not implemented
            static ui16 get_16(NIndexerCore::NIndexerDetail::TInputBuffer<TStream>*) { Y_ASSERT(false); return 0; }
            static ui32 get_32(NIndexerCore::NIndexerDetail::TInputBuffer<TStream>*) { Y_ASSERT(false); return 0; }
            static void put_8(ui8 val, NIndexerCore::NIndexerDetail::TOutputBuffer<TStream>* buf) { buf->WriteInteger(val); }
            static void put_16(ui16 val, NIndexerCore::NIndexerDetail::TOutputBuffer<TStream>* buf) { buf->Write(&val, sizeof(ui16)); }
            static void put_32(ui32 val, NIndexerCore::NIndexerDetail::TOutputBuffer<TStream>* buf) { buf->Write(&val, sizeof(ui32)); }
        };

        //! actually it is overloaded function of out_long() from library/cpp/packedtypes/longs.h
        template <typename TStream>
        inline int WritePackedLong(const i64& longVal, NIndexerCore::NIndexerDetail::TOutputBuffer<TStream>& buffer) {
            int ret;
            PACK_64(longVal, &buffer, TStreamBufferTraits<TStream>, ret);
            return ret;
        }

        template <typename TStream> class TInputIndexStream;

        //! @note TFile methods used in this class:
        //!       Close, Flush, IsOpen, Write, Seek, GetPosition (required by TOutputBuffer);
        //!       the bufSize parameter should never be equal to 0 because Write(buffer, size)
        //!       works in this case - it writes directly to the stream
        template <typename TStream>
        class TOutputIndexStream : private TNonCopyable {
            friend class TInputIndexStream<TStream>;
            TStream File; //!< underlying unbuffered stream
            TOutputBuffer<TStream> Buffer;
        public:
            typedef typename TOutputBuffer<TStream>::TDirectBuffer TDirectBuffer;
            enum EOpenMode {
                WriteOnly,
                ReadWrite
            };
            TOutputIndexStream() {
            }
            ~TOutputIndexStream() {
                // terminate program on exception.
                Close();
            }
            void Open(const char* name, EOpenMode mode = WriteOnly, size_t bufSize = 0x100000, bool directIO = false);

            // In order to ensure that all data is written you need call Close() directly
            void Close() {
                Buffer.Flush();
                Buffer.Free();
                File.Close();
            }
            void Flush() {
                Buffer.Flush();
                File.Flush();
            }
            bool IsOpen() const {
                return File.IsOpen();
            }
            bool operator!() const {
                return !IsOpen();
            }
            //! @throw yexception
            void Write(const void* buffer, size_t size) {
                if (Buffer.GetSize())
                    Buffer.Write(buffer, size);
                else
                    File.Write(buffer, size);
            }
            //! @throw yexception
            void WriteChar(int c) {
                Y_ASSERT(Buffer.GetSize()); // it function must not be used without buffer
                Buffer.WriteChar(c);
            }
            //! @throw yexception
            void WriteString(const char* str, size_t len) {
                Write(str, len);
            }
            //! @param origin    0 - SEEK_SET, 1 - SEEK_CUR, 2 - SEEK_END
            //! @note required for FAT writing: Seek(0, SEEK_SET)
            void Seek(i64 offset, int origin) {
                Buffer.Flush();
                File.Seek(offset, (SeekDir)origin);
            }
            i64 GetPosition() const {
                return Buffer.GetPosition();
            }
            //! @return length of the written value
            //! @todo rework this member function ...
            int WriteLong(i64 val) {
                return WritePackedLong(val, Buffer);
            }
            TDirectBuffer GetDirectBuffer(size_t minSize) {
                return Buffer.GetDirectBuffer(minSize);
            }
            void SetMutex(TMutex* mutex) {
                Buffer.WriteMutex = mutex;
            }
            void ResetDirect() {
                File.ResetDirect();
            }
        };

        //! the default implementation for TStream == TFile
        //! @note ONLY THIS member function must be implemented in case of using another class as TStream
        template <typename TStream>
        void TOutputIndexStream<TStream>::Open(const char* fileName, EOpenMode mode, size_t bufSize, bool directIO) {
            ::EOpenMode openMode;
            if (mode == WriteOnly)
                openMode = (CreateAlways | WrOnly);
            else if (mode == ReadWrite)
                openMode = (CreateAlways | RdWr);
            else
                ythrow yexception() << "unsupported mode";
            if (directIO)
                openMode |= DirectAligned;
            File = TFile(fileName, openMode);
            if (bufSize)
                Buffer.Allocate(File, bufSize);
        }

        //! @note TFile methods used in this class:
        //!       Close, IsOpen, GetName, GetLength, GetHandle, Read, Seek, Duplicate, GetPosition (required by TInputBuffer);
        //!       the bufSize parameter should never be equal to 0 because Read(buffer, size)
        //!       works in this case - it writes directly to the stream
        template <typename TStream>
        class TInputIndexStream : private TNonCopyable {
            TStream File; //!< underlying unbuffered stream
            const bool CloseFile;
            TInputBuffer<TStream> Buffer;
        public:
            TInputIndexStream()
                : CloseFile(true)
            {
            }
            explicit TInputIndexStream(const TOutputIndexStream<TStream>& other)
                : File(other.File)
                , CloseFile(false) // it will not close file of output stream
            {
                const size_t bufSize = other.Buffer.GetSize();
                if (bufSize)
                    Buffer.Allocate(File, bufSize);
            }
            ~TInputIndexStream() {
                Close();
            }
            void Open(const char* name, size_t bufSize = 0x100000, bool directIO = false);
            void Close() {
                if (CloseFile)
                    File.Close();
                Buffer.Free();
            }
            bool IsOpen() const {
                return File.IsOpen();
            }
            bool operator!() const {
                return !IsOpen();
            }
            //! @note required for ReadIndexInfo() and ReadFastAccessTable() only
            const char* GetName() const {
                return File.GetName().c_str();
            }
            i64 GetLength() const {
                return File.GetLength();
            }
            //! @note required for THitsBufferManagerImpl only (see library/indexfile/rdkeyit.cpp)
            size_t GetHash() const {
                return (size_t)File.GetHandle();
            }
            size_t Read(void* buffer, size_t size) {
                if (Buffer.GetSize())
                    return Buffer.Read(buffer, size);
                else
                    return File.RawRead(buffer, size);
            }
            //! @todo actually it should throw an exception
            int ReadChar() {
                Y_ASSERT(Buffer.GetSize()); // this function must not be used without buffer
                return Buffer.ReadChar();
            }
            size_t ReadString(char* buffer, size_t size) {
                return Read(buffer, size);
            }
            //! @param origin    0 - SEEK_SET, 1 - SEEK_CUR, 2 - SEEK_END
            void Seek(i64 offset, int origin) {
                Buffer.Clear();
                File.Seek(offset, (SeekDir)origin);
            }
            bool Check() const {
                return true; // other member functions check state of the file
            }
            i64 GetPosition() const {
                return Buffer.GetPosition();
            }
            TFileMap CreateFileMap() const {
                return TFileMap(File.Duplicate()); // works for TFile only
            }
            TMemoryMap CreateMapping() const {
                return TMemoryMap(File.Duplicate()); // works for TFile only
            }
        };

        //! the default implementation for TStream == TFile
        //! @note ONLY THIS member function must be implemented in case of using another class as TStream
        template <typename TStream>
        void TInputIndexStream<TStream>::Open(const char* fileName, size_t bufSize, bool directIO) {
            ::EOpenMode openMode = OpenExisting | RdOnly;
            if (directIO)
                openMode |= (Direct | DirectAligned);
            File = TFile(fileName, openMode);
            if (bufSize)
                Buffer.Allocate(File, bufSize);
        }

    } // NIndexerDetail

    typedef NIndexerDetail::TInputIndexStream<TFile> TInputFile;
    typedef NIndexerDetail::TOutputIndexStream<TFile> TOutputFile;

}
