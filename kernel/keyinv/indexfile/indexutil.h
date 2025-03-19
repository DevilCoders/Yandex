#pragma once

#include <library/cpp/yappy/yappy.h>
#include <kernel/keyinv/hitlist/hitformat.h>
#include <kernel/keyinv/hitlist/subindex.h>
#include <kernel/keyinv/hitlist/longs.h>

#include <util/generic/vector.h>
#include <util/generic/ymath.h>
#include <util/system/defaults.h>
#include <util/system/filemap.h>
#include <util/memory/blob.h>

#include <kernel/search_types/search_types.h>  // MAXKEY_LEN, MAXKEY_BUF

#include "indexstorageface.h"
#include "filestream.h"

namespace NIndexerCore {

    enum {
        PADDING_CHAR = 0
    };

    extern const ui32 versionBuffer[8];
    extern const ui32 HAS_VERSION_FIX;

    size_t Y_FORCE_INLINE KeyBlockSize(ui32 version = 0) {
        return (version & YNDEX_VERSION_FLAG_KEY_2K) ? 2048 : 4096;
    }

    struct TKeyBlockInfo {
        const char* FirstKey;
        ui32 KeyCount;
        ui64 Offset;
        ui64 KeyOffset;
    };
    struct TKeyCountLess {
        bool operator()(const TKeyBlockInfo& x, i64 number) const {
            return (ui32)x.KeyCount < number;
        }
    };
    struct TFirstKeyLess {
        bool operator()(const TKeyBlockInfo& x, const char *word) const {
            return strcmp(x.FirstKey, word) < 0;
        }
    };

    class TKeyValidator {
    public:
        enum class EValidationResult {
            Correct,
            TooLong,
            PrefixPackCharacters,
            ZeroCharacter,
            EmptyKey
        };

        static EValidationResult Validate(const char* key, const ui32 len) {
            if (len > MAXKEY_BUF) {
                return EValidationResult::TooLong;
            }
            if (!len) {
                return EValidationResult::EmptyKey;
            }
            if (key[0] >= 1 && key[0] <= 31) {
                return EValidationResult::PrefixPackCharacters;
            }
            for (ui32 i = 0; i < len; ++i) {
                const char c = key[i];
                if (c == 0 && i != len - 1) {
                    return EValidationResult::ZeroCharacter;
                }
            }
            return EValidationResult::Correct;
        }
    };

    class TStreamBlock {
        TVector<ui8>  MemBlock;
        size_t Pointer;
        enum {Junk = 32};
        size_t End;
    public:
        size_t GetPosition() const {
            return Pointer;
        }
        TStreamBlock(size_t size)
            : MemBlock(size + Junk)
            , Pointer(size_t(-1))
            , End(size)
        {
        }
        size_t GetAvail() const {
            if (Pointer < End) {
                return End - Pointer;
            }
            return 0;
        }
        void SetDirty() {
            Pointer = size_t(-1);
        }
        void SetEnd(size_t end) {
            Y_VERIFY(end + Junk <= MemBlock.size(), "indexfile library is buggy!");
            Y_ASSERT(end + Junk <= MemBlock.size());
            End = end;
        }
        void Reset() {
            Pointer = 0;
        }
        bool Dirty() const {
            return Pointer == size_t(-1);
        }
        bool Valid() const {
            return (Pointer < End);
        }
        bool Fill() const {
             return (Pointer == End);
        }
        char *GetMemory() {
            Y_VERIFY(Pointer <= End, "indexfile library is buggy!");
            return (char *)&MemBlock[Pointer];
        }
        size_t ReadPadding(ui8 padding) {
            size_t res = 0;
            while(Valid() && MemBlock[Pointer] == padding) {
                ++Pointer;
                ++res;
            }
            return res;
        }
        size_t Write(const char *data, size_t count) {
            if (Pointer + count > End)
                return 0;
            for (size_t i = 0; i < count; ++i)
                MemBlock[Pointer + i] = data[i];
            Pointer += count;
            return count;
        }
        bool Advance(size_t len) {
            Pointer += len;
            Y_VERIFY(Pointer <= End + Junk, "indexfile library is buggy!");
            return Pointer <= End;
        }
        int GetChar() {
            if (!Valid()) {
                return EOF;
            }
            return MemBlock[Pointer++];
        }
        bool PutChar(char c) {
            Y_VERIFY(!Dirty(), "indexfile library is buggy!");
            if (Pointer + 1 > End)
                return false;

            MemBlock[Pointer++] = c;
            return true;
         }
    };

    struct TInvKeyInfo {
        ui32 SizeOfFAT;
        ui32 NumberOfKeys;
        i32 NumberOfBlocks; //!< means that positions has subindex if it is negative
        static const int INFO_SIZE = 12; // useful structure data size in bytes
        TInvKeyInfo()
            : SizeOfFAT(0)
            , NumberOfKeys(0)
            , NumberOfBlocks(0)
        { }
    };

    class TMemoryMapStream: public TNonCopyable {
    public:
        TMemoryMapStream(const TMemoryMap& mapping)
            : Mapping(mapping)
            , FileMap(new TFileMap(mapping))
            , Offset(0)
        { }
        void Seek(i64 shift, SeekDir dir) {
            switch(dir) {
                case sSet:
                    SetOffset(shift);
                    break;
                case sEnd:
                    SetOffset(FileMap->Length() + shift);
                    break;
                default:
                    throw yexception() << "Invalid seek mode";
            }
        }
        /// Reads data from stream
        template <typename T>
        void Load(T* dest, size_t length) {
            Read(dest, length);
        }
        /// Reads data from stream, returns number of read bytes
        template <typename T>
        size_t Read(T* dest, size_t length) {
            FileMap->Map(Offset, length);
            memcpy(dest, FileMap->Ptr(), length);
            Offset += length;
            return length;
        }
        /// Obtains full stream length in bytes
        size_t GetLength() const noexcept{
            return FileMap->Length();
        }
        bool IsOpen() const noexcept {
            return true;
        }
        const TMemoryMap& GetMapping() {
            return Mapping;
        }
    private:
        TMemoryMapStream();

        inline void SetOffset(i64 newOfs) {
            if (newOfs < 0)
                throw yexception() << "Attempt to seek before file begin";
                //Offset = 0;
            else if (newOfs > FileMap->Length())
                throw yexception() << "Attempt to seek past end of file";
                //Offset = FileMap->Length();
            else
                Offset = newOfs;
        }

        const TMemoryMap& Mapping;
        THolder<TFileMap> FileMap;
        size_t Offset;
    };

    //! @param textBuffer    it must contain text of the previous key because keys are prefix-compressed
    //! @throw yexception - EOF unexpectedly encountered
    //! @todo return length of key text?
    bool ReadKeyText(TStreamBlock& keyStream, char* textBuffer);

    //! reads padding characters following key (actually key data)
    inline size_t ReadKeyPadding(TStreamBlock& keyStream) {
        return keyStream.ReadPadding(PADDING_CHAR);
    }

    //! @note expected that invStream::Seek() throws an exception in case of error
    template <typename TStream>
    void ReadIndexInfoFromStream(TStream& invStream, ui32& version, TInvKeyInfo& invKeyInfo) {
        invStream.Seek(-TInvKeyInfo::INFO_SIZE, sEnd);
        invStream.Load(&invKeyInfo, TInvKeyInfo::INFO_SIZE);

        const size_t fastAccessSize = invKeyInfo.SizeOfFAT + TInvKeyInfo::INFO_SIZE;

        ui32 mayBeExtraDataSize;
        ui32 mayBeVersion;
        ui32 mayBeVersionFix;
        const size_t versionSize = sizeof(mayBeExtraDataSize) + sizeof(mayBeVersion) + sizeof(mayBeVersionFix);

        // actually version must be always there...
        invStream.Seek(-(i64)(versionSize + sizeof(versionBuffer) + fastAccessSize), sEnd);

        ui32 buf[Y_ARRAY_SIZE(versionBuffer)] = { 0 };

        Y_VERIFY(invStream.Read(&mayBeExtraDataSize, sizeof(mayBeExtraDataSize)) == sizeof(mayBeExtraDataSize));
        Y_VERIFY(invStream.Read(&mayBeVersion, sizeof(mayBeVersion)) == sizeof(mayBeVersion));
        Y_VERIFY(invStream.Read(&mayBeVersionFix, sizeof(mayBeVersionFix)) == sizeof(mayBeVersionFix));
        Y_VERIFY(invStream.Read(buf, sizeof(versionBuffer)) == sizeof(versionBuffer));

        if (memcmp(buf, versionBuffer, sizeof(versionBuffer)) != 0)
            ythrow yexception() << "indexfile: bad version";
        if (mayBeVersionFix != HAS_VERSION_FIX)
            ythrow yexception() << "indexfile: bad version fix";

        version = mayBeVersion;

        invStream.Seek(0, sSet);
    }

    void FillFATBlocks(const TBlob& fat, TVector<TKeyBlockInfo>& infos, int firstBlocks[256], ui32 version, bool &isCompressed);

    TBlob ReadFastAccessTable(const TBlob& blob, ui32 version, bool &isCompressed, TVector<TKeyBlockInfo>& infos, int firstBlocks[256]);

    template <typename TStream>
    TBlob ReadFastAccessTable(TStream& file, ui32 version, bool &isCompressed, TVector<TKeyBlockInfo>& infos, const TMemoryMap& keys, int firstBlocks[256]) {
        Y_ASSERT(file.IsOpen());
        file.Seek(-TInvKeyInfo::INFO_SIZE, sEnd);
        TInvKeyInfo invKeyInfo;
        file.Load(&invKeyInfo, TInvKeyInfo::INFO_SIZE);

        invKeyInfo.NumberOfBlocks = Abs(invKeyInfo.NumberOfBlocks);

        if (!invKeyInfo.SizeOfFAT)
            return TBlob();

        i64 fastAccessOffset = invKeyInfo.SizeOfFAT + TInvKeyInfo::INFO_SIZE;

        TBlob fat = TBlob::FromMemoryMap(keys, file.GetLength() - fastAccessOffset, invKeyInfo.SizeOfFAT);

        infos.resize(size_t(invKeyInfo.NumberOfBlocks));

        FillFATBlocks(fat, infos, firstBlocks, version, isCompressed);
        return fat;
    }

    //! reads version from inv-file
    void ReadIndexInfo(const TInputFile& file, ui32& version, TInvKeyInfo& invKeyInfo);

    template <typename TStream>
    void WriteVersionData(TStream& invStream, ui32 version) {
        ui32 extraDataSize = 0;
        invStream.Write(&extraDataSize, sizeof(extraDataSize));
        invStream.Write(&version, sizeof(version));
        invStream.Write(&HAS_VERSION_FIX, sizeof(HAS_VERSION_FIX));
        invStream.Write(versionBuffer, sizeof(versionBuffer));
    }

    template <typename TStream>
    void WriteIndexStat(TStream& invStream, bool hasSubIndex, ui32 keyBufLen, ui32 numKeys, i32 numBlocks) {
        invStream.Write(&keyBufLen, 4);
        invStream.Write(&numKeys, 4);
        if (hasSubIndex)
           numBlocks = -numBlocks; // @todo if hits are additionally compressed it must be negative (see InitSubIndexInfo)
        invStream.Write(&numBlocks, 4);
    }

    //! @note key-stream must be flushed and set to the beginning and
    //!       inv-stream must be flushed and set to the end
    template <typename TReader>
    void WriteFastAccessTable(TReader& keyReader, TOutputFile& invStream) {
        ui32 keyBufLen = 0;
        ui32 numKeys = 0;
        ui32 numBlocks = 0;

        const ui32 version = keyReader.GetIndexVersion();
        WriteVersionData(invStream, version);

        bool needKeyOffset = (version & YNDEX_VERSION_FLAG_KEY_COMPRESSION);

        bool first = true;
        while(1) {
            bool needNextBlock = keyReader.NeedNextBlock();
            i64 keyOffset;
            if (needNextBlock) {
                keyOffset = keyReader.GetFilePosition();
            }
            if (!keyReader.ReadNext()) {
                break;
            }
            if (needNextBlock) {
                if (!first) {
                    // previous block counter
                    invStream.Write(&numKeys, 4);
                    keyBufLen += 4;
                }
                first = false;
                numBlocks++;
                const size_t len = strlen(keyReader.GetKeyText()) + 1;
                invStream.Write(keyReader.GetKeyText(), len);
                keyBufLen += (ui32)len;
                i64 offset = keyReader.GetOffset();
                invStream.Write(&offset, 8);
                keyBufLen += 8;

                if (needKeyOffset) {
                    invStream.Write(&keyOffset, 8);
                    keyBufLen += 8;
                }
            }
            numKeys++;
        }

        if (!first) {
            // previous incomplete block counter
            invStream.Write(&numKeys, 4);
            keyBufLen += 4;
        }

        WriteIndexStat(invStream, keyReader.GetSubIndexInfo().hasSubIndex, keyBufLen, numKeys, numBlocks);
    }

    //! @param pos      current position of key file
    //! @param len      length of key text and data
    inline bool StartNewKeyBlock(TStreamBlock &keyBlock, size_t len, size_t& padding) {
        Y_ASSERT(len > 0); // pos can be negative because it is low part of i64
        if (keyBlock.Dirty()) {
            keyBlock.Reset();
            return false;
        }
        padding = keyBlock.GetAvail();
        return len > padding;
    }

    template <typename TStream>
    inline ui32 FlushBlock(ui32 version, TStreamBlock &keyBlock, NIndexerDetail::TOutputIndexStream<TStream>& keyStream) {
        if (!keyStream)
            return 0;
        size_t size = keyBlock.GetPosition();
        if (size == 0 || keyBlock.Dirty())
            return 0;
        keyBlock.Reset();
        if (version & YNDEX_VERSION_FLAG_KEY_COMPRESSION) {
            if (version & YNDEX_VERSION_FLAG_KEY_ADDITIONALY_COMPRESSED_YAPPY) {
                TStreamBlock temp(size * 2);
                temp.Reset();
                size_t realSize = YappyCompress(keyBlock.GetMemory(), size, temp.GetMemory());
                keyStream.WriteChar(realSize & 0xff);
                keyStream.WriteChar(int(realSize >> 8));
                keyStream.WriteString(temp.GetMemory(), realSize);
                return realSize + 2;
            } else {
                Y_ASSERT(0);
            }
        } else {
            keyStream.WriteString(keyBlock.GetMemory(), size);
            return size;
        }
        return 0;
    }

    template <typename TStream>
    inline void NextBlock(ui32 version, TStreamBlock &keyBlock, NIndexerDetail::TInputIndexStream<TStream>& keyStream) {
        keyBlock.Reset();

        if (version & YNDEX_VERSION_FLAG_KEY_COMPRESSION) {
            if (version & YNDEX_VERSION_FLAG_KEY_ADDITIONALY_COMPRESSED_YAPPY) {
                int lo = 0, hi = 0;
                lo = keyStream.ReadChar();
                hi = keyStream.ReadChar();
                if (lo == EOF || hi == EOF) {
                    keyBlock.SetEnd(0);
                    return;
                }
                size_t size = lo + hi * 256;
                TStreamBlock temp(size);
                temp.Reset();
                if (size == keyStream.ReadString(temp.GetMemory(), size)) {
                    size_t realSize = YappyUnCompress(temp.GetMemory(), size, keyBlock.GetMemory());
                    keyBlock.SetEnd(realSize);
                } else {
                    Y_ASSERT(0);
                }

            } else {
                Y_ASSERT(0);
            }
        } else {
            size_t size = keyStream.ReadString(keyBlock.GetMemory(), KeyBlockSize());
            keyBlock.SetEnd(size);
        }
    }

    inline void WriteKeyPadding(TStreamBlock &streamBlock, size_t padding) {
        while (padding--)
            streamBlock.PutChar(PADDING_CHAR);
    }

    inline int GetCommonPrefixLength(const char* s1, const char* s2) {
        int len = 0;
        while (*s1 && *s2 && *s1++ == *s2++)
            len++;
        return (len > 255 ? 255 : len);
    }

    //! @param textLen     length of key text including null-terminator
    //! @throw yexception if writing fails
    inline void WriteKeyText(TStreamBlock &keyStream, const char* text, size_t textLen, int commonPrefixLen) {
        Y_ASSERT(TKeyValidator::Validate(text, textLen) == TKeyValidator::EValidationResult::Correct);
        if (commonPrefixLen >= 2) {
            if (commonPrefixLen >= 32) {
                keyStream.PutChar('\01');
            }

            keyStream.PutChar((unsigned char)commonPrefixLen);

            const size_t n = textLen - commonPrefixLen;
            if (n != keyStream.Write(text + commonPrefixLen, n))
                ythrow yexception() << "write failed";

        } else {
            if (textLen != keyStream.Write(text, textLen))
                ythrow yexception() << "write failed";
        }
    }

    inline TSubIndexInfo InitSubIndexInfo(IYndexStorage::FORMAT format, ui32 version, bool hasSubIndex) {
        if (format == IYndexStorage::FINAL_FORMAT) {
            if ((version & YNDEX_VERSION_MASK) > 1)
                return TSubIndexInfo(hasSubIndex, 64, sizeof(YxPerst), 64 * 2);
            else
                return TSubIndexInfo(hasSubIndex, 128, 16, 128 * 32);
        }
        return TSubIndexInfo();
    }

    inline TSubIndexInfo InitSubIndexInfo(IYndexStorage::FORMAT format, ui32 version) {
        const bool hasSubIndex = (format == IYndexStorage::FINAL_FORMAT);
        return InitSubIndexInfo(format, version, hasSubIndex);
    }

    inline bool ReadKeyData(TStreamBlock &streamBlock, i64& count, ui32& length, i64& pos, bool finalFormat) {
        int len = 0;
        i64 val;
        if (finalFormat) {
            len = in_long(val, streamBlock.GetMemory());
            if (!streamBlock.Advance(len))
                return false;
            count = val;
            pos += len;
        }
        len = in_long(val, streamBlock.GetMemory());
        if (!streamBlock.Advance(len))
            return false;
        length = (ui32)val;
        Y_ASSERT(length == val);
        pos += len;
        return true;
    }


    inline int GetKeyDataLength(i64 count, ui32 length, bool finalFormat) {
        int len = 0;
        if (finalFormat) {
            len += len_long(count);
        }
        i64 l64 = length;
        len += len_long(l64);
        return len;
    }

    inline void WriteKeyData(TStreamBlock &streamBlock, i64 count, ui32 length, bool finalFormat) {
        if (finalFormat) {
            size_t len = out_long(count, streamBlock.GetMemory());
            streamBlock.Advance(len);
        }
        i64 l64 = length;
        size_t len = out_long(l64, streamBlock.GetMemory());
        streamBlock.Advance(len);
        Y_ASSERT(streamBlock.Valid() || streamBlock.Fill());
    }

    TString PrintIndexVersion(ui32 version);

}
