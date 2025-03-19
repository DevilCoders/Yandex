#pragma once

#include <kernel/tarc/iface/tarcface.h>
#include <kernel/tarc/iface/tarcio.h>
#include <kernel/tarc/protos/tarc.pb.h>

#include <util/generic/hash_set.h>
#include <util/charset/wide.h>


class TBlob;
class TXRefMapped2DArray;

class IDocTextOutput
{
public:
    virtual ~IDocTextOutput() {
    }

    virtual void SetSentenceCount(ui32) {
    }

    // Sometimes you don't want to retrieve all sentences from the archive
    virtual size_t GetFirstSentence() {
        return 0;
    }
    virtual size_t GetLastSentence() {
        return static_cast<size_t>(-1);
    }

    virtual void WriteString(const TUtf16String& w, size_t sentence) = 0;
};

struct TArchiveSent;

//! interprets archive sentence text
class ISentReader {
public:
    virtual ~ISentReader() {
    }
    //! retrieves text from an archive sentence object
    virtual TUtf16String GetText(const TArchiveSent& sent, ui8 segVersion) const = 0;
};

class TSentReader : public ISentReader {
    const bool ReadAttrs;
public:
    explicit TSentReader(bool readAttrs = false)
        : ReadAttrs(readAttrs)
    {
    }
    TUtf16String GetText(const TArchiveSent& sent, ui8 segVersion) const override;
};

void PrintExtInfo(IOutputStream& os, const TBlob& extInfo, bool forceUTF8 = false);
void PrintDocText(IDocTextOutput& output, const TBlob& docText, bool mZone = false, bool wZone = false,
    bool xSent = false, bool stripTitle = false, const ISentReader& sentReader = TSentReader(), bool mZoneAttrs = false);
void PrintDocText(IOutputStream& os, const TBlob& docText, bool mZone = false, bool wZone = false,
    bool xSent = false, bool stripTitle = false, const ISentReader& sentReader = TSentReader(), bool mZoneAttrs = false);
// prints text from the specified zone only
void PrintDocText(IOutputStream& os, const TBlob& docText, EArchiveZone zoneId);

TUtf16String GetDocZoneText(const TBlob& docText, EArchiveZone zoneId);

void SaveDocTextToProto(NTextArc::TDocArcData& arcData, const TBlob& docBlob);
void SaveDocExtInfoToProto(NTextArc::TDocArcData& arcData, const TBlob& infoBlob);
TUtf16String ExtractDocTitleFromArc(const TBlob& docText);
void SaveDocTitleToProto(NTextArc::TDocArcData& arcData, const TBlob& docBlob);
THashSet<TUtf16String> ExtractDocAnchorTextsFromArc(const TBlob& docText, ui32 docId, const TXRefMapped2DArray& xref);
void SaveDocAnchorTextsToProto(NTextArc::TDocArcData& arcData, const TBlob& docBlob, ui32 docId, const TXRefMapped2DArray& xref);
