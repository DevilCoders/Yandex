#pragma once

#include <util/system/file.h>
#include <util/stream/str.h>

#include <kernel/keyinv/hitlist/hitformat.h>
#include "indexfileimpl.h"
#include "indexutil.h"
#include "fatwriter.h"

namespace NIndexerCore {

    class TInputIndexFile : public TInputIndexFileImpl<TFile> {
        friend class NIndexerDetail::TFastAccessTableWriter;
        typedef TInputIndexFileImpl<TFile> TBase;
        TInvKeyInfo InvKeyInfo;
    private:
        //! @note input index file will not take ownership of key stream and will not close it
        //!       keyStream must be open, InvStream will not be open
        TInputIndexFile(TOutputFile& keyStream, IYndexStorage::FORMAT format, ui32 version)
            : TBase(keyStream, format, version)
        {
        }
        static void SetFileFlags(TMemoryMap &fileMap) {
            Y_UNUSED(fileMap);
        }
    public:
        //! constructs index files
        explicit TInputIndexFile(IYndexStorage::FORMAT format, ui32 version = YNDEX_VERSION_CURRENT)
            : TBase(format, version)
        {
        }
        //! constructs and opens index files
        //! @note this constructor can be used if prefix is already known
        //! @throw yexception - can't open file
        TInputIndexFile(const char* prefix, IYndexStorage::FORMAT format, ui32 version = YNDEX_VERSION_CURRENT)
            : TBase(format, version)
        {
            Open(prefix);
        }
        //! constructs and opens index files
        //! @param invName    can be NULL, but in this case blocks of key-file must not be compressed (no YNDEX_VERSION_FLAG_KEY_ADDITIONALY_COMPRESSED)
        //! @note this constructor can be used if names of key- and inv-files are already known
        //! @throw yexception - can't open file
        TInputIndexFile(const char* keyName, const char* invName, IYndexStorage::FORMAT format, ui32 version = YNDEX_VERSION_CURRENT)
            : TBase(format, version)
        {
            if (invName)
                Open(keyName, invName);
            else
                OpenKeyFile(keyName);
        }
        void Open(const char* prefix) {
            Open(TString(prefix).append(KEY_SUFFIX).c_str(), TString(prefix).append(INV_SUFFIX).c_str());
        }
        //! @throw yexception - can't open file
        void Open(const char* keyName, const char* invName, int keyBufSize = INIT_FILE_BUF_SIZE, int invBufSize = INIT_FILE_BUF_SIZE, bool directIO = false) {
            TBase::Open(keyName, invName, keyBufSize, invBufSize, directIO);
            if (invName && GetFormat() == IYndexStorage::FINAL_FORMAT)
                ReadIndexInfo(GetInvStream(), Version, InvKeyInfo);
        }
        void OpenKeyFile(const char* keyName) {
            TString name(keyName);
            //need inv for version info
            if (name.size() > 3 && name.EndsWith(KEY_SUFFIX)) {
                name = name.substr(0, name.size() - 3).append(INV_SUFFIX);
                if (NFs::Exists(name)) { // don't try to open inv-file if it doesn't exist
                    Open(keyName, name.data());
                    return;
                }
            }
            Open(keyName, nullptr, INIT_FILE_BUF_SIZE, 0, false);
        }
        // In order to ensure that all data is written you need call Close() directly
        void Close() {
            InvKeyInfo.NumberOfBlocks = 0;
            TBase::Close();
        }
        i32 GetNumberOfBlocks() const {
            return InvKeyInfo.NumberOfBlocks;
        }
        ui32 GetNumberOfKeys() const {
            return InvKeyInfo.NumberOfKeys;
        }
        ui32 GetSizeOfFAT() const {
            return InvKeyInfo.SizeOfFAT;
        }
        //! @throw yexception - fast access table corrupted
        TBlob ReadFastAccessTable(TVector<TKeyBlockInfo>& table, ui32 version, bool& isCompressed, const TMemoryMap& keys, int firstBlocks[256]) {
            TFile invStream(GetInvStream().GetName(), OpenExisting | RdOnly);
            return NIndexerCore::ReadFastAccessTable(invStream, version, isCompressed, table, keys, firstBlocks);
        }
        //! @note keyFile and invFile must be already open
        TFileMap CreateKeyFileMap() const {
            return GetKeyStream().CreateFileMap();
        }
        TMemoryMap CreateInvMapping() const {
            TMemoryMap fileMap = GetInvStream().CreateMapping();
            SetFileFlags(fileMap);
            return fileMap;
        }
        TFileMap CreateInvFileMap() const {
            return GetInvStream().CreateFileMap();
        }
    };

    //! represents old interface that has CloseEx() member function
    class TOutputIndexFile : public TOutputIndexFileImpl<TFile> {
        const NIndexerDetail::TFastAccessTableWriter* FATWriter;
    public:
        using TOutputIndexFileImpl<TFile>::SetMutex;

    public:
        explicit TOutputIndexFile(IYndexStorage::FORMAT format, ui32 version = YNDEX_VERSION_CURRENT)
            : TOutputIndexFileImpl<TFile>(format, version)
            , FATWriter(nullptr)
        {
        }
        TOutputIndexFile(const char* keyName, const char* invName, IYndexStorage::FORMAT format, ui32 version = YNDEX_VERSION_CURRENT)
            : TOutputIndexFileImpl<TFile>(keyName, invName, format, version)
            , FATWriter(nullptr)
        {
        }
        void SetFATWriter(const NIndexerDetail::TFastAccessTableWriter* writer) {
            FATWriter = writer;
        }
        //! @note writes FAT using TInvKeyReader
        void CloseEx() {
            Y_ASSERT(IsOpen());
            if (GetFormat() == IYndexStorage::FINAL_FORMAT) {
                Y_ASSERT(FATWriter);
                FATWriter->WriteFAT(*this);
            }
            Close();
        }
    };


    class TOutputIndexFileFAT : public TOutputIndexFileImpl<TFile> {
    public:
        using TOutputIndexFileImpl<TFile>::SetMutex;

    public:
        explicit TOutputIndexFileFAT(IYndexStorage::FORMAT format, ui32 version = YNDEX_VERSION_CURRENT)
            : TOutputIndexFileImpl<TFile>(format, version)
            , NeedKeyOffset_(version & YNDEX_VERSION_FLAG_KEY_COMPRESSION)
        {
        }
        TOutputIndexFileFAT(const char* keyName, const char* invName, IYndexStorage::FORMAT format, ui32 version = YNDEX_VERSION_CURRENT)
            : TOutputIndexFileImpl<TFile>(keyName, invName, format, version)
            , NeedKeyOffset_(version & YNDEX_VERSION_FLAG_KEY_COMPRESSION)
        {
        }

        bool WriteKey(const char* text, int dataLen) {
            if (First_) {
                WriteVersionData(FAT_, Version);
                ProcessKeyBlock(text);
                First_ = false;
            }
            return TOutputIndexFileImpl<TFile>::WriteKey(text, dataLen);
        }

        virtual void ProcessKeyBlock(const char* text) override {
            NumBlocks_++;
            if (!First_) {
                FAT_.Write(&NumKeys_, sizeof(ui32));
                KeyBufLen_ += 4;
            }
            const size_t len = strlen(text) + 1;
            FAT_.Write(text, len);
            KeyBufLen_ += len;
            FAT_.Write(&CurInvPos_, 8);
            KeyBufLen_ += 8;
            if (NeedKeyOffset_) {
                FAT_.Write(&KeyOffset_, 8);
                KeyBufLen_ += 8;
            }
        }

        //! @note writes FAT using TInvKeyReader
        void CloseEx() {
            Y_ASSERT(IsOpen());
            if (GetFormat() == IYndexStorage::FINAL_FORMAT) {
                if (!First_) {
                    // previous incomplete block counter
                    FAT_.Write(&NumKeys_, 4);
                    KeyBufLen_ += 4;
                } else {
                    WriteVersionData(FAT_, Version);
                }
                WriteIndexStat(FAT_, true, KeyBufLen_, NumKeys_, NumBlocks_);
                WriteInvData(FAT_.Str().c_str(), FAT_.Str().size());
            }
            Close();
        }

        void AddNumKeys(ui32 delta) {
            NumKeys_ += delta;
        }

        void SetCurInvPos() {
            CurInvPos_ = InvPos_;
        }

    private:
        TStringStream FAT_;
        ui32 NumBlocks_ = 0;
        ui32 KeyBufLen_ = 0;
        bool NeedKeyOffset_;
        bool First_ = true;
    };

} // NIndexerCore
