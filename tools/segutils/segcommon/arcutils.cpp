#include "arcutils.h"

namespace NSegutils {

using namespace NDater;
using namespace NSegm;

void TArchiveWrapper::Wrap(TStringBuf archive) {
    if (TStringBuf(Archive) == archive)
        return;

    Archive = TString(archive.data(), archive.size());
    Iter.Close();
    Iter.Open(archive.data());
}

bool TArchiveWrapper::Seek(ui32 did) {
    try {
        Header = Iter.SeekToDoc(did);
        if (IsTag() && Header)
            MakeFullArchiveDocHeader(FHeader, Iter, Header);
    } catch (const yexception&) {
        return false;
    }
    return true;
}

TAttrs TArchiveWrapper::GetAttrs() const {
    if (!IsReady() || IsTag())
        return TAttrs();

    TDocDescr ext;
    TDocInfos docInfos;
    TBlob d = Iter.GetExtInfo(Header);
    ext.UseBlob(d.Data(), d.Size());
    ext.ConfigureDocInfos(docInfos);
    return TAttrs(docInfos.begin(), docInfos.end());
}

TString TArchiveWrapper::GetUrl() const {
    if (!IsReady())
        return "";

    if (IsTag())
        return FHeader.Url;

    TDocDescr ext;
    TBlob b = Iter.GetExtInfo(Header);
    ext.UseBlob(b.Data(), b.Size());
    return ext.get_url();
}

MimeTypes TArchiveWrapper::GetMime() const {
    if (!IsReady())
        return MIME_UNKNOWN;

    if (IsTag())
        return (MimeTypes) FHeader.MimeType;

    TDocDescr ext;
    TBlob b = Iter.GetExtInfo(Header);
    ext.UseBlob(b.Data(), b.Size());
    return ext.get_mimetype();
}

ECharset TArchiveWrapper::GetEncoding() const {
    if (!IsReady())
        return CODES_UNKNOWN;

    if (IsTag())
        return (ECharset) FHeader.Encoding;

    TDocDescr ext;
    TBlob b = Iter.GetExtInfo(Header);
    ext.UseBlob(b.Data(), b.Size());
    return ext.get_encoding();
}

TDaterDate TArchiveWrapper::GetBestDate() const {
    return GetBestDateFromArchive(GetAttrs());
}

TDaterStats& TArchiveWrapper::GetDaterStats() const {
    Stats.Clear();
    GetDaterStatsFromArchive(GetAttrs(), Stats);
    return Stats;
}

TString TArchiveWrapper::GetHtml() const {
    if (!IsReady())
        return "";

    if (IsTag()) {
        TBuffer b;
        GetDocTextPart(Iter.GetDocText(Header), GetMime() == MIME_HTML ? FABT_ORIGINAL
                : FABT_HTMLCONV, &b);
        return TString(b.Data(), b.Size());
    } else {
        return "";
    }
}

TArchiveSents TArchiveWrapper::GetSents(const TVector<int>& sents) const {
    TBlob b = Iter.GetDocText(Header);
    TArchiveSents outSents;
    GetSentencesByNumbers((const ui8*) b.Data(), sents, &outSents, nullptr, true);
    return outSents;
}

TArchiveMarkupZones TArchiveWrapper::GetZones() const {
    TBlob b = Iter.GetDocText(Header);
    TArchiveMarkupZones mZones;
    GetArchiveMarkupZones((const ui8*) b.Data(), &mZones);
    return mZones;
}

TSegmentSpans TArchiveWrapper::GetSegmentSpans() const {
    return NSegm::GetSegmentsFromArchive(GetZones());
}

}
