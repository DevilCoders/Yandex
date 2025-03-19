#pragma once

#include <library/cpp/charset/doccodes.h>
#include <library/cpp/mime/types/mime.h>

#include <util/charset/wide.h>

#include <util/system/defaults.h>
#include <util/system/maxlen.h> //FULLURL_MAX
#include <util/string/vector.h>

#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/generic/map.h>
#include <util/generic/buffer.h>

#include <util/str_stl.h>

#include <ctime>
#include <cstdio>  //sprintf

constexpr int DEF_ARCH_BLOB = 8192;

//Please, don't change DocUrlDescr structure without discussing it with dasha & sergey
//this will change archives format
//If you need to save additional data in archives' blob, use Extensions (see below)

struct Y_PACKED DocUrlDescr {
    ui32 HostId;
    ui32 UrlId;
    ui32 Size;
    i32 HttpModTime;
    int NextExtensionSize;
    i8 Encoding;
    ui8 MimeType;
    char Url[FULLURL_MAX + 1];
    char Padding[1];

    void Clear() {
        HostId = UrlId = 0;
        Size = 0;
        HttpModTime = 0;
        NextExtensionSize = 0;
        Encoding = CODES_WIN; // TODO: change to CODES_UNKNOWN.
        MimeType = MIME_UNKNOWN;
        memset(Url, 0, 4);
    }

    inline size_t size() const {
        size_t sz = sizeof(DocUrlDescr) + strlen(Url) - FULLURL_MAX;
#ifdef _must_align4_
        sz = (sz + 3)&~ 3;
#endif
        return sz;
    }

    DocUrlDescr() { Clear(); }
};

//Extensions: can be added into blob after DocUrlDescr and Url
enum ExtensionType {
    Last = -1,
    SiteInfo, // It seems that this extensions has never set. But sometimes being read. TODO: remove.
    PicInfo,
    DocInfo,
    BuildInfo,
    BinDocInfo
};

//dummy header for all extensions
struct ExtensionInfo {
    i32  NextExtensionSize;
    i32  OwnSize;
    ExtensionType EType;
};

//sample of real extension
struct PicInfoExt {
    ExtensionInfo EInfo;
    ui32 ThumbnailId;
    ui32 NaturalImageSize;
    ui16 NaturalWidth;
    ui16 NaturalHeight;
    ui16 ThumbnailWidth;
    ui16 ThumbnailHeight;
    ui8 NaturalImageFormat;
    ui8 ThumbnailImageFormat;
    char Urls[1];               //two of them, delimited by \0 :1. Picture url (now empty, cuase == DocUrlDescr.Url )
                                //2. Url of Html contaning picture

    bool Print(char* buffer, unsigned  bufferSize) const
    {
       if (bufferSize < 7 /*delims*/ + strlen(Urls) + strlen(Urls + strlen(Urls) + 1) + 10 /*uint print size*/
            + 6 * 5  /*ushort*/ + 1 /* \0 */)
            return false;
        sprintf(buffer, "%" PRIu32 "\t%" PRIu16 "\t%" PRIu16 "\t%" PRIu16 "\t%" PRIu16 "\t%d\t%d\t%s\t%s\t%lu", ThumbnailId, NaturalWidth, NaturalHeight,
                            ThumbnailWidth, ThumbnailHeight, (int)NaturalImageFormat, (int)ThumbnailImageFormat,
                            Urls, Urls + strlen(Urls) + 1, (unsigned long)NaturalImageSize);
        return true;
    }
};

struct TAdditionalSiteInfo {
    ExtensionInfo EInfo;
    char Host[HOST_MAX + 1];
};

struct TDocInfoExt {
    ExtensionInfo EInfo;
    char Info[1];
};

struct TBinDocInfoExt {
    ExtensionInfo EInfo;
    ui32 BinFormat;
};

struct TBuildInfoExt {
    ExtensionInfo EInfo;
    ui32 Revision;
    char SvnUrl[FULLURL_MAX + 1];

    TBuildInfoExt() {
        SvnUrl[0] = '\0';
    }

    size_t GetSize() const {
        return sizeof(ExtensionInfo) + sizeof(Revision) + strlen(SvnUrl) + 1;
    }
};

using TDocInfos = THashMap< const char*, const char*, THash<const char*>, TEqualTo<const char*>>;
using TDocInfosPtr = TAtomicSharedPtr<TDocInfos>;

///////////////////////////////////////////////////////////////////////////////

class IDocInfosBuilderCallback {
public:
    virtual ~IDocInfosBuilderCallback() = default;;
    virtual void OnBuildProperty(const char* name, const char* value) = 0;
    virtual void OnAfterBuildDocProps(const ui32& /*count*/) {};
};

///////////////////////////////////////////////////////////////////////////////

class TDocDescr {
private:
    DocUrlDescr* UrlDescr;
    TBuffer*     Buffer;
    unsigned     ExtensionsLen;

private:
    ExtensionInfo* GetFirstExtension() const;

    ExtensionInfo* GetNextExtension(const ExtensionInfo* e) const;

    ExtensionInfo* GetLastExtension() const;

    void ResizeBuffer(size_t size, bool reserveOnly);

public:
    TDocDescr();

    //! Takes all data from blob but makes DocDescr readonly.
    void UseBlob(const void *blob, unsigned blobSize);

    //! Allows writing blobs.
    void UseBlob(TBuffer* buffer);

    void Clear();

    bool IsAvailable() const {
        return UrlDescr != nullptr;
    }

    bool IsReadOnly() const {
        return Buffer == nullptr;
    }

    DocUrlDescr& GetMutableUrlDescr() {
        Y_ENSURE(IsAvailable() && !IsReadOnly());
        return *UrlDescr;
    }

    size_t CalculateBlobSize() const;

    bool HasUrlSet() const {
        return strlen(UrlDescr->Url) > 0;
    }

    //
    // Extensions.
    //

    int AddExtension(ExtensionType EType, size_t extensionSize, const void *eata = nullptr);

    const void* GetExtension(ExtensionType type, size_t extensionNum = 0) const;

    int RemoveExtension(ExtensionType EType);

    void RemoveExtensions();

    //! @note removes all extensions before copying
    void CopyExtensions(const TDocDescr& other);

    //! Finds number of EType extensions
    int FindCount(ExtensionType type) const;

    int FindCount(const char *extName) const;

    bool get_extention_print_data(const char *Extname, int ext_num, int /*mode*/, char *buffer, unsigned bufferSize) const;

    //
    // DocUrlDescription.
    //

    void SetUrl(const char* url);
    void SetUrl(const TStringBuf& url);
    void SetEncoding(ECharset encoding);
    void SetUrlAndEncoding(const char* url, ECharset encoding);

    ui32 ConfigureDocInfos(IDocInfosBuilderCallback* cb) const;

    ui32 ConfigureDocInfos(THashMap<TString, TString>& docInfos) const;
    ui32 ConfigureDocInfos(TDocInfos& docInfos) const;

    //! @note clears this object before copying
    void CopyUrlDescr(const TDocDescr& other);

    ui32 get_hostid(/*int nurl = 0*/) const {
        return IsAvailable() ? UrlDescr->HostId : 0;
    }

    ui32 get_urlid(/*int nurl = 0*/) const {
        return IsAvailable() ? UrlDescr->UrlId : 0;
    }

    size_t get_size(/*int nurl = 0*/) const {
        return IsAvailable() ? UrlDescr->Size : 0;
    }

    time_t get_mtime(/*int nurl = 0*/) const {
        return IsAvailable() ? UrlDescr->HttpModTime : 0;
    }

    ECharset get_encoding(/*int nurl = 0*/) const {
        return IsAvailable() ? (ECharset)UrlDescr->Encoding : CODES_UNKNOWN;
    }

    MimeTypes get_mimetype(/*int nurl = 0*/) const {
        return IsAvailable() ? (MimeTypes)UrlDescr->MimeType : MIME_UNKNOWN;
    }

    const char* get_url(/*int nurl = 0*/) const {
        return  IsAvailable() ? UrlDescr->Url : "";
    }

    const char* get_host() const;

    void set_hostid(ui32 setHostId) {
        if (IsAvailable() && !IsReadOnly())
            UrlDescr->HostId = setHostId;
    }

    void set_urlid(ui32 setUrlId) {
        if (IsAvailable() && !IsReadOnly())
            UrlDescr->UrlId = setUrlId;
    }

    void set_size(size_t setSize) {
        if (IsAvailable() && !IsReadOnly())
            UrlDescr->Size = (ui32)setSize;
    }

    void set_mtime(time_t setTime) {
        if (IsAvailable() && !IsReadOnly())
            UrlDescr->HttpModTime = (i32)setTime;
    }

    void set_mimetype(MimeTypes setMime) {
        if (IsAvailable() && !IsReadOnly())
            UrlDescr->MimeType = (ui8)setMime;
    }
};

///////////////////////////////////////////////////////////////////////////////

class TDocInfoExtWriter {
public:
    TDocInfoExtWriter() = default;
    void Add(const char* name, const char* value);
    void Write(TDocDescr& DD) const;
    void Clear();

private:
    // TODO use THashMap and const char*: strings exist all the time while the class works (check it out just in case)
    typedef TMap<TString, TSimpleSharedPtr<TVector<TString>> > TData;
    TData Data;
};

///////////////////////////////////////////////////////////////////////////////

class TDocArchive {
private:
    TVector<TUtf16String> Passages;
    TVector<int> Breaks;

public:
    TBuffer Blob;
    int Blobsize;
    bool Exists;
    ui8 NoText;
    TVector<TUtf16String> TitleSentences;
    TVector<TUtf16String> AbstractSentences;
    TVector<TString> Attrs;
    TUtf16String MetaDescription;
    TUtf16String RawAbstract;
    TUtf16String Title;
    unsigned AbstractLen;
    bool IsFake;

    TDocArchive();
    ~TDocArchive();
    void Clear();
    char* GetBlob() {
        Blob.Reserve(Blobsize);
        return Blob.Data();
    }

    TUtf16String& GetPassage(size_t index)  {
        return Passages[index];
    }

    const TUtf16String& GetPassage(size_t index) const {
        return Passages[index];
    }

    const TVector<TUtf16String>& GetPassages() const {
        return Passages;
    }

    size_t GetPassagesSize() const {
        return Passages.size();
    }

    int GetBreak(size_t index) {
        return Breaks[index];
    }

    void AddPassage(int nBreak, const TUtf16String& passage);
    void ClearPassages();
    void RemovePassage(size_t index);
    void RemovePassages(size_t from, size_t to);
    void SwapPassagesAndAttrs(TDocArchive& da);
};
