#pragma once

#include <tools/snipmake/common/common.h>

#include <util/folder/path.h>
#include <util/generic/string.h>
#include <util/generic/noncopyable.h>
#include <contrib/libs/re2/re2/re2.h>
#include <util/stream/file.h>

class TRE2Replacer {
public:
    TRE2Replacer(const TString pattern, const TString replacement);
    TString ReplaceAll(const TString& text) const;

private:
    const re2::RE2 Pattern;
    const TString Replacement;
};

class TFileStorage {
    friend class TSnippetIterator;
public:
    TFileStorage(const TString& storageRoot);
    ~TFileStorage();

    // Returns storage_hashID
    TString StoreJSONSnipPool(const TString& jsonFileName, bool isRCA);
    TString StoreSnipPool(const TString& xmlSnipFileName);
    TString StoreRawFile(const TString& rawFileName);
    TString StoreSerp(const TString& xmlSerpFileName);

    TString GetRawFile(const TString& fileId);

    void DeleteFile(const TString& fileId);

private:
    TFsPath GetFilepath(const TString& poolId) const;
    TFsPath GetFilename(const TString& poolId) const;

private:
    typedef TMultiMap<TString, TString> TSteamMMap;

private:
    TString ReqSnip2ProtoBuf(const NSnippets::TReqSnip& snipXmlObj);
    TString PreprocessMarkup(const TString& text);
    TString ReplaceBigUTF8Chars(const TString& text);
    void ReadSnippets(TSteamMMap& dest, NSnippets::ISnippetsIterator& snipIt);
    TString WriteSnippets(const TSteamMMap& sortedSnippets);

private:
    TFsPath StorageRoot;
    const TRE2Replacer BrCutter;
    const TRE2Replacer StrongToBReplacer;
    const TRE2Replacer DoubleSpaceReplacer;
    const TString Unsupported;
};

class TSnippetIterator : private TNonCopyable {
public:
    // If endSnipId not specified,
    // returns snippets until EOF
    TSnippetIterator(const TFileStorage& storage, const TString& poolId,
        size_t startSnipId = 0, size_t endSnipId = -1);
    ~TSnippetIterator();

    bool Valid() const;
    TString Value() const;
    void Next();
    size_t GetSnipCount() const;

private:
    void UpdateCurValue();
    void SetInvalid();

private:
    THolder<TUnbufferedFileInput> InputSnipFile;
    THolder<TUnbufferedFileInput> InputIndexFile;
    size_t CurSnipId;
    size_t EndSnipId;
    ui64 Offset;
    size_t SnipCount;
    ui64 InputSnipFileSize;
    TString CurValue;
};

