/// author@ vvp@ Victor Ploshikhin
/// created: Oct 25, 2011 6:27:34 PM
/// see: BUKI-1289

#include "doc_url_index.h"

#include <kernel/indexdoc/omnidoc.h>

#include <library/cpp/on_disk/2d_array/array2d.h>

#include <util/generic/vector.h>
#include <util/folder/dirut.h>

class TDocUrlIndexReaderImpl {
private:
    FileMapped2DArray<ui32, char> UrlsArr;

public:
    explicit TDocUrlIndexReaderImpl(const TString& indexFileName)
        : UrlsArr(indexFileName)
    {
    }

    TStringBuf Get(const ui32 docId) const /*throw(yexception)*/
    {
        if ( docId >= UrlsArr.size() ) {
            ythrow yexception() << "docId " << docId << " is greater than doc-url index size " << UrlsArr.size() << "\n";
        }
        return TStringBuf(UrlsArr.GetBegin(docId), UrlsArr.GetEnd(docId));
    }

    size_t Size() const
    {
        return UrlsArr.size();
    }
};

TDocUrlIndexReader::TDocUrlIndexReader() = default;
TDocUrlIndexReader::~TDocUrlIndexReader() = default;

TDocUrlIndexReader::TDocUrlIndexReader(const TString& indexFileName)
    : Impl(new TDocUrlIndexReaderImpl(indexFileName))
{
}

TStringBuf TDocUrlIndexReader::Get(const ui32 docId) const /*throw(yexception)*/
{
    Y_VERIFY(Impl.Get() != nullptr, "TDocUrlIndexReader::Impl is empty!");
    return Impl->Get(docId);
}

size_t TDocUrlIndexReader::Size() const
{
    Y_VERIFY(Impl.Get() != nullptr, "TDocUrlIndexReader::Impl is empty!");
    return Impl->Size();
}

TDocUrlIndexManager::TDocUrlIndexManager(const TString& oldIndexPath, const TDocOmniWadIndex* newReader)
    : NewReader(newReader)
{
    if (NewReader)
        return;
    if (NFs::Exists(oldIndexPath)) {
        OldReader.Reset(new TDocUrlIndexReader(oldIndexPath));
    }
}

size_t TDocUrlIndexManager::Size() const {
    if (NewReader)
        return NewReader->Size();
    Y_ASSERT(OldReader.Get());
    return OldReader->Size();
}

TStringBuf TDocUrlIndexManager::Get(const ui32 docId, TOmniUrlAccessor* accessor) const {
    if (NewReader && accessor) {
        return NewReader->GetByAccessor(docId, accessor);
    }
    Y_ASSERT(OldReader.Get());
    return OldReader->Get(docId);
}

TDocUrlIndexManager::~TDocUrlIndexManager() = default;
