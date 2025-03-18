#pragma once

#include "data_utils.h"
#include "unpacker.h"

#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/farcface.h>
#include <kernel/tarc/iface/tarcface.h>
#include <kernel/tarc/iface/tarcio.h>
#include <kernel/tarc/markup_zones/unpackers.h>

#include <util/string/type.h>


namespace NSegutils {
typedef THashMap<TString, TString> TAttrs;

class TArchiveWrapper {
    TString Archive;
    TArchiveIterator Iter;
    TFullArchiveDocHeader FHeader;
    TArchiveHeader* Header;
    mutable NDater::TDaterStats Stats;

public:
    TArchiveWrapper() :
        Iter(), Header() {
    }

    ui32 GetId() const {
        return Header ? Header->DocId : 0;
    }

    TString GetArchive() const {
        return Archive;
    }

    bool IsReady() const {
        return Header;
    }

    void Reset() {
        Header = nullptr;
        Iter.Seek(0);
    }

    bool IsTag() const {
        return Archive.EndsWith("tag");
    }

    bool Next() {
        Header = Iter.NextAuto();

        if (IsTag() && Header)
            MakeFullArchiveDocHeader(FHeader, Iter, Header);

        return Header;
    }

    bool SeekAny(TStringBuf any) {
        if (IsNumber(any))
            SeekId(any);
        return SeekUrl(any);
    }

    bool SeekId(TStringBuf id) {
        return Seek(FromString<ui32> (id));
    }

    bool SeekUrl(TStringBuf /*url*/) {
        return false; // todo
    }

    template<typename TSpan>
    TVector<TSpan> GetSpans(EArchiveZone zone) const {
        const TVector<TArchiveZoneSpan> & spans = GetZones().GetZone(zone).Spans;
        return NSegm::GetSpansFromZone<TSpan>(spans.begin(), spans.size());
    }

    void Wrap(TStringBuf archive);
    bool Seek(ui32 did);

    TAttrs GetAttrs() const;
    TString GetUrl() const;
    MimeTypes GetMime() const;
    ECharset GetEncoding() const;
    NDater::TDaterDate GetBestDate() const;
    NDater::TDaterStats& GetDaterStats() const;
    ui32 GetSegmentVersion() const;

    TString GetHtml() const;

    NSegm::TArchiveSents GetSents(const TVector<int>& sents = TVector<int>()) const;
    TArchiveMarkupZones GetZones() const;

    NSegm::TSegmentSpans GetSegmentSpans() const;
};

}
