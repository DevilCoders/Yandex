#include "docdescr.h"

#include <util/memory/tempbuf.h>
#include <util/generic/yexception.h>
#include <util/generic/buffer.h>

static_assert(sizeof(DocUrlDescr) == 1308, "expect sizeof(DocUrlDescr) == 1308");

///////////////////////////////////////////////////////////////////////////////
// Initialization.
///////////////////////////////////////////////////////////////////////////////

TDocDescr::TDocDescr()
    : UrlDescr(nullptr)
    , Buffer(nullptr)
    , ExtensionsLen(0)
{ }

void TDocDescr::UseBlob(const void* blob, unsigned blobSize) {
    if (blobSize == 0)
        return;

    Buffer    = nullptr; //< Means readonly
    UrlDescr  = (struct DocUrlDescr*) blob;
    ExtensionsLen = 0;

    if (UrlDescr->NextExtensionSize != 0) {
        ExtensionInfo* currentExt = GetFirstExtension();
        while (currentExt) {
            ExtensionsLen += currentExt->OwnSize;
            currentExt = GetNextExtension(currentExt);
        }
    }

    // Check blob
    if (CalculateBlobSize() > blobSize) {
        ythrow yexception() << "Bad data";
    }
}

void TDocDescr::UseBlob(TBuffer* buffer) {
    Y_ASSERT(buffer);
    if (!buffer) {
        return;
    }

    Buffer = buffer;

    // If the current url description points to external blob.
    // Then copy existent.
    if (UrlDescr && Buffer->Size() == 0) {
        if ((const char*)UrlDescr != Buffer->Data()) {
            // move existing BLOB data to the buffer we have just set
            size_t blobSize = CalculateBlobSize();
            Buffer->Append((const char*)UrlDescr, blobSize);
            UrlDescr = (struct DocUrlDescr*) Buffer->Data();
            // ExtensionsLen stay correct.
        }
    } else {
        // Use a new buffer.
        if (Buffer->Size() == 0) {
            ResizeBuffer(DEF_ARCH_BLOB, true);
            UrlDescr = (struct DocUrlDescr*) Buffer->Data();
            UrlDescr->Clear();
        } else {
            UrlDescr = (struct DocUrlDescr*) Buffer->Data();
        }

        // Recount extensions state
        if (UrlDescr->NextExtensionSize != 0) {
            ExtensionInfo* currentExt = GetFirstExtension();
            while (currentExt) {
                ExtensionsLen += currentExt->OwnSize;
                currentExt = GetNextExtension(currentExt);
            }
        }
    }

    ResizeBuffer(CalculateBlobSize(), false);
}

void TDocDescr::Clear() {
    if (IsAvailable())
        UrlDescr->Clear();

    ExtensionsLen = 0;

    if (!IsReadOnly()) {
        ResizeBuffer(CalculateBlobSize(), false);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Extensions.
///////////////////////////////////////////////////////////////////////////////

ExtensionInfo* TDocDescr::GetFirstExtension() const {
    if (UrlDescr->NextExtensionSize != 0) {
        return (ExtensionInfo*)((char*)UrlDescr + UrlDescr->size());
    } else {
        return nullptr;
    }
}

ExtensionInfo* TDocDescr::GetNextExtension(const ExtensionInfo* e) const {
    if (e->NextExtensionSize != 0) {
        return (ExtensionInfo*)((char*)e + e->OwnSize);
    } else {
        return nullptr;
    }
}

ExtensionInfo* TDocDescr::GetLastExtension() const {
    ExtensionInfo* currentExt = GetFirstExtension();
    while (!!currentExt) {
        if (currentExt->NextExtensionSize == 0)
            return currentExt;

        currentExt = GetNextExtension(currentExt);
    }
    return nullptr;
}

int TDocDescr::AddExtension(ExtensionType type, size_t extensionSize, const void *data) {
    Y_ASSERT(data);

    if (!IsAvailable() || IsReadOnly() || extensionSize == 0)
        return 1;

    // Prepare place
    if (CalculateBlobSize() + extensionSize >= Buffer->Capacity()) {
        ResizeBuffer(CalculateBlobSize() + extensionSize, true);
    }

    ExtensionInfo* currentExt = GetLastExtension();
    if (currentExt) {
        currentExt->NextExtensionSize = extensionSize;
        currentExt = (ExtensionInfo*)((char*)currentExt + currentExt->OwnSize);
    } else {
        UrlDescr->NextExtensionSize = extensionSize;
        currentExt = (ExtensionInfo*)((char*)UrlDescr + UrlDescr->size());
    }

    memcpy((void *)currentExt, data, extensionSize);

    currentExt->OwnSize = extensionSize;
    currentExt->NextExtensionSize = 0;
    currentExt->EType = type;

    ExtensionsLen += extensionSize;
    ResizeBuffer(CalculateBlobSize(), false);

    return 0;
}

const void* TDocDescr::GetExtension(ExtensionType type, size_t extensionNum) const {
    if (!IsAvailable())
        return nullptr;

    ExtensionInfo* currentExt = GetFirstExtension();
    size_t currentIdx = 0;
    while (currentExt) {
        if (currentExt->EType == type) {
            if (currentIdx == extensionNum) {
                return currentExt;
            } else {
                ++currentIdx;
            }
        }

        currentExt = GetNextExtension(currentExt);
    }
    return nullptr;
}

int TDocDescr::FindCount(ExtensionType type) const {
    if (type == Last  || !IsAvailable() || !UrlDescr->NextExtensionSize)
        return 0;

    int count = 0;
    ExtensionInfo* currentExt = GetFirstExtension();
    while (currentExt) {
        if (currentExt->EType == type) {
            ++count;
        }

        currentExt = GetNextExtension(currentExt);
    }
    return count;
}

int TDocDescr::FindCount(const char *extName) const {
    if (!strcmp(extName, "PicInfo")) {
        return FindCount(PicInfo);
    }
    return 0;
}

int TDocDescr::RemoveExtension(ExtensionType type) {
    if (!IsAvailable() || IsReadOnly())
        return 1;

    ExtensionInfo* currentExt = GetFirstExtension();
    ExtensionInfo* prevExt    = nullptr;

    while (currentExt) {
        if (currentExt->EType == type) {
            if (!prevExt) {
                if (currentExt->NextExtensionSize == 0) {
                    UrlDescr->NextExtensionSize = 0;
                    ExtensionsLen = 0;
                    break;
                } else {
                    ExtensionInfo* copied = GetNextExtension(currentExt);
                    UrlDescr->NextExtensionSize = copied->OwnSize;
                    ExtensionsLen -= currentExt->OwnSize;
                    memmove((void*)currentExt, copied, Buffer->Pos() - (const char*) copied);
                }
            } else {
                ExtensionsLen -= currentExt->OwnSize;

                if (currentExt->NextExtensionSize == 0) {
                    prevExt->NextExtensionSize = 0;
                    break;
                } else {
                    ExtensionInfo* copied = GetNextExtension(currentExt);
                    prevExt->NextExtensionSize = copied->OwnSize;
                    memmove((void*)currentExt, copied, Buffer->Pos() - (const char*) copied);
                }
            }
        } else {
            prevExt    = currentExt;
            currentExt = GetNextExtension(currentExt);
        }
    }

    ResizeBuffer(CalculateBlobSize(), false);
    return 0;
}

void TDocDescr::RemoveExtensions() {
    if (!IsAvailable() || IsReadOnly())
        return;

    ExtensionsLen = 0;
    UrlDescr->NextExtensionSize = 0;
    ResizeBuffer(CalculateBlobSize(), false);
}

bool TDocDescr::get_extention_print_data(const char *extname, int extNum, int /*mode*/, char *buffer, unsigned bufferSize) const
{
    if (!stricmp(extname, "PicInfo")) {
        PicInfoExt* picExt = (PicInfoExt*) GetExtension(PicInfo, extNum);
        if (picExt) {
            return picExt->Print(buffer, bufferSize);
        }
    }

    return false;
}

void TDocDescr::CopyExtensions(const TDocDescr& other) {
    if (!IsAvailable() || IsReadOnly())
        return;

    RemoveExtensions();

    const size_t newSize = CalculateBlobSize() + other.ExtensionsLen;
    if (newSize >= Buffer->Capacity()) {
        ResizeBuffer(newSize, true);
    }

    const ExtensionInfo* copied = other.GetFirstExtension();
    while (copied) {
        AddExtension(copied->EType, copied->OwnSize, copied);

        copied = other.GetNextExtension(copied);
    }
}

///////////////////////////////////////////////////////////////////////////////
// DocUrlDescription.
///////////////////////////////////////////////////////////////////////////////

void TDocDescr::SetEncoding(ECharset encoding) {
    if (IsAvailable() && !IsReadOnly()) {
        UrlDescr->Encoding = (i8)encoding;
    }
}

void TDocDescr::SetUrl(const char* url) {
    return SetUrl(TStringBuf(url, strnlen(url, FULLURL_MAX)));
}

void TDocDescr::SetUrl(const TStringBuf& url) {
    if (IsAvailable() && !IsReadOnly()) {
        // trying to preserve ancient logic:
        //  - although Url can keep FULLURL_MAX + 1 chars,
        //    max url length that can be stored is FULLURL_MAX - 1
        //  - zeroing using 4 zeroes
        const size_t len = Min<size_t>(url.size(), FULLURL_MAX - 1);
        memcpy(UrlDescr->Url, url.data(), len);
        const size_t zeroLen = Min(size_t(4), FULLURL_MAX - len);
        memset(UrlDescr->Url + len, 0, zeroLen);
    }
}

void TDocDescr::SetUrlAndEncoding(const char* url, ECharset encoding) {
    //! Check that it is possible.
    if (IsAvailable() && !IsReadOnly() && ExtensionsLen == 0) {
        SetEncoding(encoding);
        SetUrl(url);
        ResizeBuffer(CalculateBlobSize(), false);
    } else {
        ythrow yexception() << "It's not a good idea: it can damage the object";
    }
}

void TDocDescr::CopyUrlDescr(const TDocDescr& other) {
    if (!IsAvailable() || IsReadOnly())
        return;

    UrlDescr->Clear();

    if (other.IsAvailable()) {
        ResizeBuffer(other.UrlDescr->size(), true);
        memcpy(UrlDescr, other.UrlDescr, other.UrlDescr->size());
        UrlDescr->NextExtensionSize = 0;
    }
}

namespace {

    template <typename TDocInfos>
    class TDocInfosBuilderCallbackHash: public IDocInfosBuilderCallback {
    private:
        TDocInfos& DI;

    public:
        TDocInfosBuilderCallbackHash(TDocInfos& di): DI(di) {
            Y_ASSERT(DI.empty());
        }

        void OnBuildProperty(const char* name, const char* value) override {
            DI.insert(typename TDocInfos::value_type(name, value));
        }

        void OnAfterBuildDocProps(const ui32& count) override {
            Y_UNUSED(count);
            Y_ASSERT((size_t)count == DI.size());
        }
    };

} // namespace // anonymous


ui32 TDocDescr::ConfigureDocInfos(THashMap<TString, TString>& docInfos) const {
    TDocInfosBuilderCallbackHash<THashMap<TString, TString>> di(docInfos);
    return ConfigureDocInfos(&di);
}

ui32 TDocDescr::ConfigureDocInfos(TDocInfos& docInfos) const {
    TDocInfosBuilderCallbackHash<TDocInfos> di(docInfos);
    return ConfigureDocInfos(&di);
}

ui32 TDocDescr::ConfigureDocInfos(IDocInfosBuilderCallback* cb) const {
    ui32 count = 0;
    const TDocInfoExt* docInfo = (const TDocInfoExt*) GetExtension(DocInfo);
    if (docInfo) {
        const char* names = docInfo->Info;
        ui32 nlen = *((ui32*)names);
        names += sizeof(nlen);

        const char* values = names + nlen;
        size_t len = 0;
        while (len < nlen) {
            cb->OnBuildProperty(names, values);
            size_t names_shift = strlen(names)+1;
            len += names_shift;
            names += names_shift;
            values += strlen(values)+1;
            ++count;
        }
    }
    cb->OnAfterBuildDocProps(count);
    return count;
}

const char* TDocDescr::get_host() const {
    if (!IsAvailable()) {
        return "";
    }

    TAdditionalSiteInfo* sInfo = (TAdditionalSiteInfo*)  GetExtension(SiteInfo);
    if (sInfo) {
        return sInfo->Host;
    } else {
        return "";
    }
}

size_t TDocDescr::CalculateBlobSize() const {
    return IsAvailable() ? UrlDescr->size() + ExtensionsLen : 0;
}

void TDocDescr::ResizeBuffer(size_t size, bool reserveOnly) {
    Y_ASSERT(!IsReadOnly());

    if (Buffer->Capacity() >= size && reserveOnly)
        return;

    if (reserveOnly)
        Buffer->Reserve(size);
    else
        Buffer->Proceed(size);

    UrlDescr = (DocUrlDescr*)Buffer->Data();
}

/*****************************TDocInfoExtWriter********************************/

void TDocInfoExtWriter::Add(const char* name, const char* value)
{
    Y_ASSERT(name && *name);
    Y_ASSERT(value);

    TData::iterator it = Data.find(name);
    if (it == Data.end()) {
        Data[name] = new TVector<TString>();
        it = Data.find(name);
    }
    it->second->push_back(value);
}

void TDocInfoExtWriter::Write(TDocDescr& DD) const
{
    Y_ASSERT(DD.IsAvailable());
    if (!Data.empty()) {
        TBuffer names;
        TBuffer values;

        for (TData::const_iterator i = Data.begin(), mi = Data.end(); i != mi; ++i) {
            names.Append(i->first.c_str(), i->first.size() + 1);
            TString value = JoinStrings(*i->second, "\t");
            values.Append(value.c_str(), value.size() + 1);
        }

        ui32 hlen = sizeof(ExtensionInfo);
        ui32 nlen = (ui32)names.Size();
        ui32 vlen = (ui32)values.Size();
        Y_ASSERT(vlen);

        size_t len = hlen + sizeof(nlen) + nlen + vlen;
        TTempBuf tempBuf(len);
        char* blob = tempBuf.Data();
        char* begin = blob + hlen;
        memcpy(begin, &nlen, sizeof(nlen));
        begin += sizeof(nlen);
        memcpy(begin, names.Begin(), nlen);
        begin += nlen;
        memcpy(begin, values.Begin(), vlen);
        DD.AddExtension(DocInfo, len, blob);
    }
}

void TDocInfoExtWriter::Clear()
{
    Data.clear();
}

/************************************ DocArchive ************************************/

TDocArchive::TDocArchive()
{
    Clear();
}

TDocArchive::~TDocArchive()
{
}

void TDocArchive::Clear() {
    Exists = false;
    NoText = 0;
    Blobsize = 0;
    Blob.Clear();
    Blob.Append('\0');
    TitleSentences.clear();
    ClearPassages();
    Attrs.clear();
    MetaDescription.clear();
    RawAbstract.clear();
    AbstractLen = 0;
    AbstractSentences.clear();
    IsFake = false;
}

void TDocArchive::AddPassage(int nBreak, const TUtf16String& passage)
{
    Y_ASSERT(Passages.size() == Breaks.size());
    Passages.push_back(passage);
    Breaks.push_back(nBreak);
}

void TDocArchive::ClearPassages()
{
    Passages.clear();
    Breaks.clear();
}

void TDocArchive::RemovePassage(size_t index)
{
    Passages.erase(Passages.begin() + index);
    Breaks.erase(Breaks.begin() + index);
}

void TDocArchive::RemovePassages(size_t from, size_t to)
{
    Passages.erase(Passages.begin() + from, Passages.begin() + to);
    Breaks.erase(Breaks.begin() + from, Breaks.begin() + to);
}

void TDocArchive::SwapPassagesAndAttrs(TDocArchive& da)
{
    Y_ASSERT(da.Passages.size() == da.Breaks.size());
    Y_ASSERT(Passages.size() == Breaks.size());
    Passages.swap(da.Passages);
    Breaks.swap(da.Breaks);
    Attrs.swap(da.Attrs);
}
