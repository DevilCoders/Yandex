#pragma once

#include <util/generic/noncopyable.h>
#include <util/folder/dirut.h>
#include <util/system/filemap.h>

#include <kernel/keyinv/hitlist/hitformat.h>
#include <kernel/keyinv/hitlist/longs.h>

#include <kernel/search_types/search_types.h>

#include "indexutil.h"
#include "filestream.h"

namespace NIndexerCore {

    enum {
        INIT_FILE_BUF_SIZE = 0x100000
    };

    constexpr TStringBuf KEY_SUFFIX = "key";
    constexpr TStringBuf INV_SUFFIX = "inv";

    //! base class for input/output index files
    class TIndexFileBase : private TNonCopyable {
    private:
        const IYndexStorage::FORMAT Format;

    protected:
        ui32 Version;   //!< TInputIndexFile assigns it after reading version from file
        TStreamBlock KeyBlock;

    protected:
        TIndexFileBase(IYndexStorage::FORMAT format, ui32 version)
            : Format(format)
            , Version(version)
            , KeyBlock(NIndexerCore::KeyBlockSize(version))
        {
        }
    public:
        IYndexStorage::FORMAT GetFormat() const {
            return Format;
        }
        //! @attention in case of TInputIndexFile::OpenKeyFile() version is not read from inv-file
        ui32 GetVersion() const {
            return Version;
        }
    };

    namespace NIndexerDetail {
        class TFastAccessTableWriter;
    }

    //! reads data from key/inv-files, use prefix compression for key text
    template <typename TStream>
    class TInputIndexFileImpl : public TIndexFileBase {
        friend class NIndexerDetail::TFastAccessTableWriter;
    private:
        NIndexerDetail::TInputIndexStream<TStream> KeyStream;
        NIndexerDetail::TInputIndexStream<TStream> InvStream;
    protected:
        //! @note input index file will not take ownership of key stream and will not close it;
        //!       keyStream must be open, InvStream will not be open
        TInputIndexFileImpl(NIndexerDetail::TOutputIndexStream<TStream>& keyStream, IYndexStorage::FORMAT format, ui32 version)
            : TIndexFileBase(format, version)
            , KeyStream(keyStream)
        {
            Y_ASSERT(KeyStream.IsOpen());
        }
        const NIndexerDetail::TInputIndexStream<TStream>& GetKeyStream() const {
            return KeyStream;
        }
        const NIndexerDetail::TInputIndexStream<TStream>& GetInvStream() const {
            return InvStream;
        }
    public:
        //! @note version is read from inv-file in case of FINAL_FORMAT
        //!       if NumberOfBlocks is less than 0 TSubIndexInfo::hasSubIndex is assigned to true in TInvKeyReader
        //!       in case of PORTION_FORMAT version must be specified but hasSubIndex will be false by default
        explicit TInputIndexFileImpl(IYndexStorage::FORMAT format, ui32 version = YNDEX_VERSION_CURRENT)
            : TIndexFileBase(format, version)
        {
        }
        //! constructs and opens index files
        //! @param invName    can be NULL, but in this case blocks of key-file must not be compressed (no YNDEX_VERSION_FLAG_KEY_ADDITIONALY_COMPRESSED)
        //! @note this constructor can be used if names of key- and inv-files are already known
        //! @throw yexception - can't open file
        TInputIndexFileImpl(const char* keyName, const char* invName, IYndexStorage::FORMAT format, ui32 version = YNDEX_VERSION_CURRENT)
            : TIndexFileBase(format, version)
        {
            Open(keyName, invName);
        }
        //! @note keyFile and invFile must be already open
        //!       in some cases invFile is not required
        TInputIndexFileImpl(const TStream& keyFile, const TStream& invFile, IYndexStorage::FORMAT format, ui32 version = YNDEX_VERSION_CURRENT)
            : TIndexFileBase(format, version)
            , KeyStream(keyFile)
            , InvStream(invFile)
        {
        }
        ~TInputIndexFileImpl() {
            Close();
        }
        //! @throw yexception - can't open file
        void Open(const char* keyName, const char* invName, int keyBufSize = INIT_FILE_BUF_SIZE, int invBufSize = INIT_FILE_BUF_SIZE, bool directIO = false) {
            KeyStream.Open(keyName, keyBufSize, directIO);
            if (invName)
                InvStream.Open(invName, invBufSize, directIO);
        }
        void Close() {
            KeyStream.Close();
            InvStream.Close();
        }
        bool IsOpen() const {
            // check only KeyStream because TInputIndexFile(TFileStream&, ..) or OpenKeyFile() can be called
            return KeyStream.IsOpen();
        }
        bool operator!() const {
            return !IsOpen();
        }
        //! @note this member function is used by TReadKeysIteratorImpl and it would be better to make it private
        //!       and add template <class T> friend class TReadKeysIteratorImpl;
        NIndexerDetail::TInputIndexStream<TStream>& GetInvFile() {
            return InvStream;
        }
        NIndexerDetail::TInputIndexStream<TStream>& GetKeyFile() {
            return KeyStream;
        }
        i64 GetInvFileLength() const {
            return InvStream.GetLength();
        }
        void NextBlock() {
            NIndexerCore::NextBlock(Version, KeyBlock, KeyStream);
        }
        i64 GetFilePosition() {
            return KeyStream.GetPosition();
        }
        bool Valid() {
            return KeyBlock.Valid();
        }
        void SetDirty() {
            KeyBlock.SetDirty();
        }
        //! @param textBuffer    it must contain text of the previous key because keys are prefix-compressed
        bool ReadKeyText(char* textBuffer) {
            return NIndexerCore::ReadKeyText(KeyBlock, textBuffer);
        }
        bool ReadKeyData(i64& val) {
            size_t len = in_long(val, KeyBlock.GetMemory());
            return KeyBlock.Advance(len);
        }
        void ReadKeyPadding() {
            NIndexerCore::ReadKeyPadding(KeyBlock);
        }
        //! @attention offset in inv-file must be changed as well, see TInvKeyReader::SkipTo()
        void SeekKeyFile(i64 offset) {
            Y_ASSERT(offset >= 0 && ((Version & YNDEX_VERSION_FLAG_KEY_COMPRESSION) || (offset % KeyBlockSize()) == 0));
            KeyStream.Seek(offset, SEEK_SET);
            NextBlock();
        }
    };

    //! writes data to key/inv-files, use prefix compression for key text
    //! @note defines structure of key and inv files
    template <typename TStream>
    class TOutputIndexFileImpl : public TIndexFileBase {
        friend class NIndexerDetail::TFastAccessTableWriter;
    private:
        char PrevText[MAXKEY_BUF]; //!< to pack prefix compressed key text
        NIndexerDetail::TOutputIndexStream<TStream> KeyStream;
        NIndexerDetail::TOutputIndexStream<TStream> InvStream;

    protected:
        i64 KeyOffset_ = 0;
        i64 InvPos_ = 0;
        i64 CurInvPos_ = 0;
        ui32 NumKeys_ = 0;

    private:
        //! @param textLen      includes null-terminator
        int GetTotalKeyLength(int textLen, int commonPrefixLen, int dataLen) {
            if (commonPrefixLen >= 2 && commonPrefixLen <= 31)
                return textLen - commonPrefixLen + 1 + dataLen;
            else if (commonPrefixLen >= 32)
                return textLen - commonPrefixLen + 2 + dataLen;
            else
                return textLen + dataLen;
        }
   public:
        explicit TOutputIndexFileImpl(IYndexStorage::FORMAT format, ui32 version = YNDEX_VERSION_CURRENT)
            : TIndexFileBase(format, version)
        {
            PrevText[0] = 0;
        }
        TOutputIndexFileImpl(const char* keyName, const char* invName, IYndexStorage::FORMAT format, ui32 version = YNDEX_VERSION_CURRENT)
            : TIndexFileBase(format, version)
        {
            PrevText[0] = 0;
            Open(keyName, invName);
        }
        //! @note keyFile and invFile must be already open
        TOutputIndexFileImpl(const TStream& keyFile, const TStream& invFile, IYndexStorage::FORMAT format, ui32 version = YNDEX_VERSION_CURRENT)
            : TIndexFileBase(format, version)
            , KeyStream(keyFile)
            , InvStream(invFile)
        {
            PrevText[0] = 0;
        }
        virtual ~TOutputIndexFileImpl() {
            // terminate program on exception.
            if (IsOpen())
                Close();
        }
        void Open(const char* prefix) {
            Open(prefix, INIT_FILE_BUF_SIZE, INIT_FILE_BUF_SIZE);
        }
        void Open(const char* prefix, int keyBufSize, int invBufSize, bool directIO = false) {
            Open(TString(prefix).append(KEY_SUFFIX).c_str(), TString(prefix).append(INV_SUFFIX).c_str(), keyBufSize, invBufSize, directIO);
        }
        void Open(const char* keyName, const char* invName, bool directIO = false) {
            Open(keyName, invName, INIT_FILE_BUF_SIZE, INIT_FILE_BUF_SIZE, directIO);
        }
        //! @throw yexception - can't open file
        void Open(const char* keyName, const char* invName, int keyBufSize, int invBufSize, bool directIO = false) {
            KeyStream.Open(keyName, (GetFormat() == IYndexStorage::FINAL_FORMAT ? TOutputFile::ReadWrite : TOutputFile::WriteOnly), keyBufSize, false);
            InvStream.Open(invName, TOutputFile::WriteOnly, invBufSize, directIO);
            NumKeys_ = 0;
            KeyOffset_ = 0;
            InvPos_ = 0;
            CurInvPos_ = 0;
        }
        // In order to ensure that all data is written you need call Close() directly
        void Close() {
            Flush();
            KeyStream.Close();
            InvStream.Close();
        }
        bool IsOpen() const {
            return KeyStream.IsOpen() && InvStream.IsOpen();
        }
        bool operator!() const {
            return !IsOpen();
        }
        ui32 FlushKeyBlock() {
            return FlushBlock(Version, KeyBlock, KeyStream);
        }
        void Flush() {
            FlushKeyBlock();
            KeyBlock.SetDirty();
            KeyStream.Flush();
            InvStream.Flush();
        }
        bool StartBlockIfNeeded(const char* text, int dataLen, int& textLen, int& commonPrefixLen) {
            Y_ASSERT(text);
            textLen = int(strlen(text) + 1); // including null-terminator
            if (textLen == 1)
                ythrow yexception() << "key text is empty";
            if (textLen > MAXKEY_BUF)
                ythrow yexception() << "key text is too long (" << textLen - 1 << " characters)";
            commonPrefixLen = GetCommonPrefixLength(text, PrevText);
            int keyLen = GetTotalKeyLength(textLen, commonPrefixLen, dataLen);

            size_t padding;
            if (StartNewKeyBlock(KeyBlock, keyLen, padding)) {
                commonPrefixLen = 0; // the first key in block has no prefix compression
                WriteKeyPadding(KeyBlock, padding);
                KeyOffset_ += FlushKeyBlock();
                ProcessKeyBlock(text);
                return true;
            }
            return false;
        }

        //! @param text     key text including null-terminator must not be longer than MAXKEY_BUF
        //! @param dataLen  length of key data, it is used to calculate whether a new block should be started
        //! @note data must be written just after writing key text
        //! @exception yexception if key text is empty of too long
        bool WriteKey(const char* text, int dataLen) {
            int textLen;
            int commonPrefixLen;
            bool blockStarted = StartBlockIfNeeded(text, dataLen, textLen, commonPrefixLen);
            WriteKeyText(KeyBlock, text, textLen, commonPrefixLen);
            NumKeys_++;
            strcpy(PrevText, text);
            CurInvPos_ = InvPos_;
            return blockStarted;
        }

        void ResetPrevText() {
            PrevText[0] = 0;
        }

        virtual void ProcessKeyBlock(const char*) {
        }

        void WriteKeyData(const void* data, size_t size) {
            KeyStream.Write(data, size);
            KeyOffset_ += size;
        }
        //! @throw yexception - if write failed
        void WriteInvData(const void* data, size_t size) {
            InvStream.Write(data, size);
            InvPos_ += size;
        }
        //! writes raw data into inv stream
        int WriteInvData(SUPERLONG data) {
            int written = InvStream.WriteLong(data);
            InvPos_ += written;
            return written;
        }
        //! returns packed size of number
        //! @note see comment for WriteKeyData
        int GetKeyDataLen(i64 val) {
            return len_long(val);
        }
        //! writes number into key stream
        //! @note actually this function should be the same as WriteInvData(void*, size_t)
        //!       and TOutputIndexFile should not know anything about format of key data
        //!       but because of out_long() it was made so
        void WriteKeyData(i64 val) {
            Y_ASSERT(KeyStream.IsOpen());
            KeyBlock.Advance(out_long(val, KeyBlock.GetMemory()));
            Y_ASSERT(KeyBlock.Valid() || KeyBlock.Fill());
        }
        typename NIndexerDetail::TOutputIndexStream<TStream>::TDirectBuffer GetDirectInvBuffer(size_t minSize) {
            return InvStream.GetDirectBuffer(minSize);
        }
        void SetMutex(TMutex* mutex) {
            InvStream.SetMutex(mutex);
        }
    };

}
