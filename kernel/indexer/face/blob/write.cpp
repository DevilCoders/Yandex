#include "serialize.h"
#include <kernel/indexer/face/blob/directtext.pb.h>

#include <util/system/defaults.h>
#include <util/system/yassert.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite.h>

namespace NIndexerCore {

///////////////////////////////////////////////////////////////////////////////

using namespace google::protobuf::io;
using namespace google::protobuf::internal;

///////////////////////////////////////////////////////////////////////////////

static ui32 DocInfoByteSize(const TDocInfoEx& docInfo) {
    const ui32 size
        // optional uint32 DocId = 1
        = 1 + WireFormatLite::UInt32Size(docInfo.DocId)
        // optional uint64 FeedId = 2
        + 1 + WireFormatLite::UInt64Size(docInfo.FeedId)
        // optional uint32 UrlFlags = 3
        + 1 + WireFormatLite::UInt32Size(docInfo.UrlFlags)
        // optional sint32 Encoding = 4
        + 1 + WireFormatLite::SInt32Size(docInfo.DocHeader->Encoding)
        // optional uint32 Language = 5
        + 1 + WireFormatLite::UInt32Size(docInfo.DocHeader->Language)
        // optional uint32 Language2 = 6
        + 1 + WireFormatLite::UInt32Size(docInfo.DocHeader->Language2)
        // optional sint32 IndexDate = 7
        + 1 + WireFormatLite::SInt32Size(docInfo.DocHeader->IndexDate)
        // optional uint32 MimeType = 8
        + 1 + WireFormatLite::UInt32Size((ui8)docInfo.DocHeader->MimeType)
        // optional string Url = 9
        + 1 + WireFormatLite::LengthDelimitedSize(strlen(docInfo.DocHeader->Url) + 1);

    return size;
}

// TDirectText::TCounts
static ui32 CountsByteSize(const TDirectTextData2& data) {
    const ui32 size
        // optional uint32 Entries = 1
        = 1 + WireFormatLite::UInt64Size(data.EntryCount)
        // optional uint32 Zones = 2
        + 1 + WireFormatLite::UInt64Size(data.ZoneCount)
        // optional uint32 ZoneAttrs = 3
        + 1 + WireFormatLite::UInt64Size(data.ZoneAttrCount)
        // optional uint32 SentAttrs = 4
        + 1 + WireFormatLite::UInt64Size(data.SentAttrCount);

    return size;
}

// TDirectText::TSpace
static ui32 SpaceByteSize(const TDirectTextSpace& space) {
    const ui32 size
        // optional bytes Space = 1
        = 1 + WireFormatLite::LengthDelimitedSize(space.Length * sizeof(wchar16))
        // optional uint32 BreakType = 2
        + 1 + WireFormatLite::UInt32Size(static_cast<ui32>(space.Type));

    return size;
}

// TDirectText::TToken
static ui32 TokenByteSize(const TLemmatizedToken& token) {
    ui32 size
        // optional uint32 Lang = 3
        = 1 + WireFormatLite::UInt32Size(token.Lang)
        // optional uint32 Flags = 4
        + 1 + WireFormatLite::UInt32Size(token.Flags)
        // optional uint32 Joins = 5
        + 1 + WireFormatLite::UInt32Size(token.Joins)
        // optional uint32 FormOffset = 6
        + 1 + WireFormatLite::UInt32Size(token.FormOffset)
        // optional sint32 TermCount = 7
        + 1 + WireFormatLite::SInt32Size(token.TermCount)
        // optional double Weight = 8
        + 1 + WireFormatLite::kDoubleSize
        // optional uint32 Prefix = 13
        + 1 + WireFormatLite::UInt32Size(token.Prefix);

        // optional bytes LemmaText = 1
        if (token.LemmaText) {
            size += 1 + WireFormatLite::LengthDelimitedSize(strlen(token.LemmaText) + 1);
        }
        // optional bytes FormaText = 2
        if (token.FormaText) {
            size += 1 + WireFormatLite::LengthDelimitedSize(strlen(token.FormaText) + 1);
        }
        // optional bytes StemGram = 9
        if (token.StemGram) {
            size += 1 + WireFormatLite::LengthDelimitedSize(strlen(token.StemGram) + 1);
        }
        // optional uint32 FlexGramsCount = 10
        if (token.GramCount) {
            size += 1 + WireFormatLite::UInt32Size(token.GramCount);
        }
        // repeated bytes FlexGrams = 11
        for (size_t i = 0; i < token.GramCount; ++i) {
            size += 1 + WireFormatLite::LengthDelimitedSize(strlen(token.FlexGrams[i]) + 1);
        }
        // optional bool IsBastard = 12
        if (token.IsBastard) {
            size += 1 + WireFormatLite::kBoolSize;
        }

    return size;
}

// TDirectText::TEntry
static ui32 EntryByteSize(const TDirectTextEntry2& e) {
    ui32 size
        // optional fixed32 Posting = 1;
        = 1 + WireFormatLite::kFixed32Size
        // optional uint32 Offset = 2;
        + 1 + WireFormatLite::UInt32Size(e.OrigOffset);
        // optional bytes OriginalToken = 3;
        if (e.Token) {
            size += 1 + WireFormatLite::LengthDelimitedSize((e.Token.size() + 1) * sizeof(wchar16));
        }
        // optional uint32 TokensCount = 4;
        if (e.LemmatizedTokenCount) {
             size += 1 + WireFormatLite::UInt32Size(e.LemmatizedTokenCount);
        }
        // optional uint32 SpacesCount = 5;
        if (e.SpaceCount) {
            size += 1 + WireFormatLite::UInt32Size(e.SpaceCount);
        }
        // repeated TToken Tokens = 6;
        for (size_t i = 0; i < e.LemmatizedTokenCount; ++i) {
            size += 1 + WireFormatLite::LengthDelimitedSize(TokenByteSize(e.LemmatizedToken[i]));
        }
        // repeated TSpace Spaces = 7;
        for (size_t i = 0; i < e.SpaceCount; ++i) {
            size += 1 + WireFormatLite::LengthDelimitedSize(SpaceByteSize(e.Spaces[i]));
        }

    return size;
}

// TDirectText::TSentAttr
static ui32 SentAttrByteSize(const TDirectTextSentAttr& sa) {
    const ui32 size
        // optional uint32 Sent = 1
        = 1 + WireFormatLite::UInt32Size(sa.Sent)
        // optional bytes Name = 2
        + 1 + WireFormatLite::LengthDelimitedSize(sa.Attr.size())
        // optional bytes Value = 3;
        + 1 + WireFormatLite::LengthDelimitedSize(sa.Value.size());

    return size;
}

// TDirectMarkup::TPosition
static ui32 PositionByteSize(ui16 sent, ui16 word) {
    const ui32 size
        // optional uint32 Sent = 1
        = 1 + WireFormatLite::UInt32Size(sent)
        // optional uint32 Word = 2
        + 1 + WireFormatLite::UInt32Size(word);

    return size;
}

// TDirectMarkup::TAttrEntry
static ui32 AttrEntryByteSize(const TDirectAttrEntry& ae) {
    ui32 size
        // optional TPosition Position = 1
        = 1 + WireFormatLite::LengthDelimitedSize(PositionByteSize(ae.Sent, ae.Word))
        // optional bytes Value = 2
        + 1 + WireFormatLite::LengthDelimitedSize((ae.AttrValue.size() + 1) * sizeof(wchar16));
        // optional bool NoFollow = 3
        if (ae.NoFollow) {
            size += 1 + WireFormatLite::kBoolSize;
        }

    return size;
}

// TDirectMarkup::TAttribute
static ui32 ZoneAttrByteSize(const TDirectTextZoneAttr& za) {
    ui32 size
        // optional uint32 Type = 1
        = 1 + WireFormatLite::UInt32Size(za.AttrType)
        // optional bytes Name = 2
        + 1 + WireFormatLite::LengthDelimitedSize(za.AttrName.size() + 1);
        // repeated TAttrEntry Entries = 3;
        for (size_t i = 0; i < za.EntryCount; ++i) {
            size += 1 + WireFormatLite::LengthDelimitedSize(AttrEntryByteSize(za.Entries[i]));
        }

    return size;
}

// TDirectMarkup::TZoneSpan
static ui32 ZoneSpanByteSize(const TZoneSpan& span) {
    const ui32 size
        // optional TPosition Begin = 1
        = 1 + WireFormatLite::LengthDelimitedSize(PositionByteSize(span.SentBeg, span.WordBeg))
        // optional TPosition End = 2
        + 1 + WireFormatLite::LengthDelimitedSize(PositionByteSize(span.SentEnd, span.WordEnd));

    return size;
}

// TDirectMarkup::TZone
static ui32 ZoneByteSize(const TDirectTextZone& zone) {
    ui32 size
        // optional uint32 Type = 1
        = 1 + WireFormatLite::UInt32Size(zone.ZoneType)
        // optional bytes Name = 2
        + 1 + WireFormatLite::LengthDelimitedSize(zone.Zone.size() + 1);
        // repeated TZoneSpan Spans = 3
        for (size_t i = 0; i < zone.SpanCount; ++i) {
            size += 1 + WireFormatLite::LengthDelimitedSize(ZoneSpanByteSize(zone.Spans[i]));
        }

    return size;
}

// TDirectText
static ui32 DirectTextByteSize(const TDocInfoEx& docInfo, const TDirectTextData2& dt) {
    ui32 size
        // optional TDocInfo DocInfo = 1
        = 1 + WireFormatLite::LengthDelimitedSize(DocInfoByteSize(docInfo))
        // optional TCounts Counts = 2
        + 1 + WireFormatLite::LengthDelimitedSize(CountsByteSize(dt));
        // repeated TEntry Entries = 3
        for (size_t i = 0; i < dt.EntryCount; ++i) {
            size += 1 + WireFormatLite::LengthDelimitedSize(EntryByteSize(dt.Entries[i]));
        }
        // repeated TDirectMarkup.TZone Zones = 4
        for (size_t i = 0; i < dt.ZoneCount; ++i) {
            size += 1 + WireFormatLite::LengthDelimitedSize(ZoneByteSize(dt.Zones[i]));
        }
        // repeated TDirectMarkup.TAttribute ZoneAttrs = 5
        for (size_t i = 0; i < dt.ZoneAttrCount; ++i) {
            size += 1 + WireFormatLite::LengthDelimitedSize(ZoneAttrByteSize(dt.ZoneAttrs[i]));
        }
        // repeated TSentAttr SentAttrs = 6
        for (size_t i = 0; i < dt.SentAttrCount; ++i) {
            size += 1 + WireFormatLite::LengthDelimitedSize(SentAttrByteSize(dt.SentAttrs[i]));
        }

    return size;
}

// TDirectMarkup
static ui32 DirectMarkupByteSize(const TVector<TDirectTextZoneAttr>& attrs, const TVector<TDirectTextZone>& zones) {
    ui32 size
        = 0;
        // repeated TAttribute Attributes = 1
        for (size_t i = 0; i < attrs.size(); ++i) {
            size += 1 + WireFormatLite::LengthDelimitedSize(ZoneAttrByteSize(attrs[i]));
        }
        // repeated TZone Zones = 2
        for (size_t i = 0; i < zones.size(); ++i) {
            size += 1 + WireFormatLite::LengthDelimitedSize(ZoneByteSize(zones[i]));
        }

    return size;
}

///////////////////////////////////////////////////////////////////////////////

static inline ui8* WriteBytesField(const int field, const ui8* data, ui32 len, ui8* output) {
    output = CodedOutputStream::WriteVarint32ToArray(
                WireFormatLite::MakeTag(field, WireFormatLite::WIRETYPE_LENGTH_DELIMITED), output);
    output = CodedOutputStream::WriteVarint32ToArray(len, output);
    output = CodedOutputStream::WriteRawToArray(data, len, output);
    return output;
}

static inline ui8* WriteRecord(const int field, ui32 size, ui8* output) {
    output = CodedOutputStream::WriteVarint32ToArray(
                WireFormatLite::MakeTag(field, WireFormatLite::WIRETYPE_LENGTH_DELIMITED), output);
    output = CodedOutputStream::WriteVarint32ToArray(size, output);
    return output;
}

static ui8* WriteDocInfo(const TDocInfoEx& docInfo, ui8* p) {
    // optional uint32 DocId = 1
    p = WireFormatLite::WriteUInt32ToArray(1, docInfo.DocId, p);
    // optional uint64 FeedId = 2
    p = WireFormatLite::WriteUInt64ToArray(2, docInfo.FeedId, p);
    // optional uint32 UrlFlags = 3
    p = WireFormatLite::WriteUInt32ToArray(3, docInfo.UrlFlags, p);
    // optional sint32 Encoding = 4
    p = WireFormatLite::WriteSInt32ToArray(4, docInfo.DocHeader->Encoding, p);
    // optional uint32 Language = 5
    p = WireFormatLite::WriteUInt32ToArray(5, docInfo.DocHeader->Language, p);
    // optional uint32 Language2 = 6
    p = WireFormatLite::WriteUInt32ToArray(6, docInfo.DocHeader->Language2, p);
    // optional sint32 IndexDate = 7
    p = WireFormatLite::WriteSInt32ToArray(7, docInfo.DocHeader->IndexDate, p);
    // optional uint32 MimeType = 8
    p = WireFormatLite::WriteUInt32ToArray(8, (ui8)docInfo.DocHeader->MimeType, p);
    // optional string Url = 9
    p = WriteBytesField(9, (const ui8*)docInfo.DocHeader->Url, strlen(docInfo.DocHeader->Url) + 1, p);

    return p;
}

static ui8* WriteCounts(const TDirectTextData2& dt, ui8* p) {
    // optional uint32 Entries = 1
    p = WireFormatLite::WriteUInt64ToArray(1, dt.EntryCount, p);
    // optional uint32 Zones = 2
    p = WireFormatLite::WriteUInt64ToArray(2, dt.ZoneCount, p);
    // optional uint32 ZoneAttrs = 3
    p = WireFormatLite::WriteUInt64ToArray(3, dt.ZoneAttrCount, p);
    // optional uint32 SentAttrs = 4
    p = WireFormatLite::WriteUInt64ToArray(4, dt.SentAttrCount, p);

    return p;
}

static ui8* WriteToken(const TLemmatizedToken& token, ui8* p) {
    // optional uint32 Lang = 3
    p = WireFormatLite::WriteUInt32ToArray(3, token.Lang, p);
    // optional uint32 Flags = 4
    p = WireFormatLite::WriteUInt32ToArray(4, token.Flags, p);
    // optional uint32 Joins = 5
    p = WireFormatLite::WriteUInt32ToArray(5, token.Joins, p);
    // optional uint32 FormOffset = 6
    p = WireFormatLite::WriteUInt32ToArray(6, token.FormOffset, p);
    // optional sint32 TermCount = 7
    p = WireFormatLite::WriteSInt32ToArray(7, token.TermCount, p);
    // optional double Weight = 8
    p = WireFormatLite::WriteDoubleToArray(8, token.Weight, p);
    // optional uint32 Prefix = 13
    p = WireFormatLite::WriteUInt32ToArray(13, token.Prefix, p);

    // optional bytes LemmaText = 1
    if (token.LemmaText) {
        p = WriteBytesField(1, (const ui8*)token.LemmaText, strlen(token.LemmaText) + 1, p);
    }
    // optional bytes FormaText = 2
    if (token.FormaText) {
        p = WriteBytesField(2, (const ui8*)token.FormaText, strlen(token.FormaText) + 1, p);
    }
    // optional bytes StemGram = 9
    if (token.StemGram) {
        p = WriteBytesField(9, (const ui8*)token.StemGram, strlen(token.StemGram) + 1, p);
    }
    // optional uint32 FlexGramsCount = 10
    if (token.GramCount) {
        p = WireFormatLite::WriteUInt32ToArray(10, token.GramCount, p);
    }
    // repeated bytes FlexGrams = 11
    for (size_t i = 0; i < token.GramCount; ++i) {
        p = WriteBytesField(11, (const ui8*)token.FlexGrams[i], strlen(token.FlexGrams[i]) + 1, p);
    }
    // optional bool IsBastard = 12
    if (token.IsBastard) {
        p = WireFormatLite::WriteBoolToArray(12, token.IsBastard, p);
    }

    return p;
}

static ui8* WriteSpace(const TDirectTextSpace& space, ui8* p) {
    // optional bytes Space = 1
    p = WriteBytesField(1, (ui8*)space.Space, space.Length * sizeof(wchar16), p);
    // optional uint32 BreakType = 2
    p = WireFormatLite::WriteUInt32ToArray(2, static_cast<ui32>(space.Type), p);

    return p;
}

static ui8* WriteEntry(const TDirectTextEntry2& e, ui8* p) {
    // optional fixed32 Posting = 1
    p = WireFormatLite::WriteFixed32ToArray(1, e.Posting, p);
    // optional uint32 Offset = 2
    p = WireFormatLite::WriteUInt32ToArray(2, e.OrigOffset, p);
    // optional bytes OriginalToken = 3
    if (e.Token) {
        p = WriteBytesField(3, (const ui8*)e.Token.data(), (e.Token.size() + 1) * sizeof(wchar16), p);
    }
    // optional uint32 TokensCount = 4
    if (e.LemmatizedTokenCount) {
        p = WireFormatLite::WriteUInt32ToArray(4, e.LemmatizedTokenCount, p);
    }
    // optional uint32 SpacesCount = 5
    if (e.SpaceCount) {
        p = WireFormatLite::WriteUInt32ToArray(5, e.SpaceCount, p);
    }
    // repeated TToken Tokens = 6
    for (size_t i = 0; i < e.LemmatizedTokenCount; ++i) {
        p = WriteRecord(6, TokenByteSize(e.LemmatizedToken[i]), p);
        p = WriteToken(e.LemmatizedToken[i], p);
    }
    // repeated TSpace Spaces = 7
    for (size_t i = 0; i < e.SpaceCount; ++i) {
        p = WriteRecord(7, SpaceByteSize(e.Spaces[i]), p);
        p = WriteSpace(e.Spaces[i], p);
    }

    return p;
}

static ui8* WritePosition(ui16 sent, ui16 word, ui8* p) {
    // optional uint32 Sent = 1
    p = WireFormatLite::WriteUInt32ToArray(1, sent, p);
    // optional uint32 Word = 2
    p = WireFormatLite::WriteUInt32ToArray(2, word, p);

    return p;
}

static ui8* WriteAttrEntry(const TDirectAttrEntry& ae, ui8* p) {
    // optional TPosition Position = 1
    p = WriteRecord(1, PositionByteSize(ae.Sent, ae.Word), p);
    p = WritePosition(ae.Sent, ae.Word, p);
    // optional bytes Value = 2
    p = WriteBytesField(2, (const ui8*)ae.AttrValue.data(), (ae.AttrValue.size() + 1) * sizeof(wchar16), p);
    // optional bool NoFollow = 3
    if (ae.NoFollow) {
        p = WireFormatLite::WriteBoolToArray(3, ae.NoFollow, p);
    }

    return p;
}

static ui8* WriteZoneAttr(const TDirectTextZoneAttr& za, ui8* p) {
    // optional uint32 Type = 1
    p = WireFormatLite::WriteUInt32ToArray(1, za.AttrType, p);
    // optional bytes Name = 2
    p = WriteBytesField(2, (const ui8*)za.AttrName.data(), za.AttrName.size() + 1, p);
    // repeated TAttrEntry Entries = 3;
    for (size_t i = 0; i < za.EntryCount; ++i) {
        p = WriteRecord(3, AttrEntryByteSize(za.Entries[i]), p);
        p = WriteAttrEntry(za.Entries[i], p);
    }

    return p;
}

static ui8* WriteZoneSpan(const TZoneSpan& span, ui8* p) {
    // optional TPosition Begin = 1
    p = WriteRecord(1, PositionByteSize(span.SentBeg, span.WordBeg), p);
    p = WritePosition(span.SentBeg, span.WordBeg, p);
    // optional TPosition End = 2
    p = WriteRecord(2, PositionByteSize(span.SentEnd, span.WordEnd), p);
    p = WritePosition(span.SentEnd, span.WordEnd, p);

    return p;
}

static ui8* WriteZone(const TDirectTextZone& zone, ui8* p) {
    // optional uint32 Type = 1
    p = WireFormatLite::WriteUInt32ToArray(1, zone.ZoneType, p);
    // optional bytes Name = 2
    p = WriteBytesField(2, (const ui8*)zone.Zone.data(), zone.Zone.size() + 1, p);
    // repeated TZoneSpan Spans = 3
    for (size_t i = 0; i < zone.SpanCount; ++i) {
        p = WriteRecord(3, ZoneSpanByteSize(zone.Spans[i]), p);
        p = WriteZoneSpan(zone.Spans[i], p);
    }

    return p;
}

static ui8* WriteSentAttr(const TDirectTextSentAttr& sa, ui8* p) {
    // optional uint32 Sent = 1
    p = WireFormatLite::WriteUInt32ToArray(1, sa.Sent, p);
    // optional bytes Name = 2
    p = WriteBytesField(2, (const ui8*)sa.Attr.c_str(), sa.Attr.size() + 1, p);
    // optional bytes Value = 3
    p = WriteBytesField(3, (const ui8*)sa.Value.c_str(), sa.Value.size() + 1, p);

    return p;
}

TBuffer SerializeDirectText(const TDocInfoEx& docInfo, const TDirectTextData2& dt) {
    const ui32 size = DirectTextByteSize(docInfo, dt);
    TBuffer buf(size);
    ui8* p = reinterpret_cast<ui8*>(buf.Data());

    // optional TDocInfo DocInfo = 1
    p = WriteRecord(1, DocInfoByteSize(docInfo), p);
    p = WriteDocInfo(docInfo, p);
    // optional TCounts Counts = 2
    p = WriteRecord(2, CountsByteSize(dt), p);
    p = WriteCounts(dt, p);
    // repeated TEntry Entries = 3
    for (size_t i = 0; i < dt.EntryCount; ++i) {
        p = WriteRecord(3, EntryByteSize(dt.Entries[i]), p);
        p = WriteEntry(dt.Entries[i], p);
    }
    // repeated TDirectMarkup.TZone Zones = 4
    for (size_t i = 0; i < dt.ZoneCount; ++i) {
        p = WriteRecord(4, ZoneByteSize(dt.Zones[i]), p);
        p = WriteZone(dt.Zones[i], p);
    }
    // repeated TDirectMarkup.TAttribute ZoneAttrs = 5
    for (size_t i = 0; i < dt.ZoneAttrCount; ++i) {
        p = WriteRecord(5, ZoneAttrByteSize(dt.ZoneAttrs[i]), p);
        p = WriteZoneAttr(dt.ZoneAttrs[i], p);
    }
    // repeated TSentAttr SentAttrs = 6
    for (size_t i = 0; i < dt.SentAttrCount; ++i) {
        p = WriteRecord(6, SentAttrByteSize(dt.SentAttrs[i]), p);
        p = WriteSentAttr(dt.SentAttrs[i], p);
    }

    Y_ASSERT((char*)p == buf.Data() + size);
    buf.Advance(size);
    return buf;
}

TBuffer SerializeMarkup(const TVector<TDirectTextZoneAttr>& attrs, const TVector<TDirectTextZone>& zones) {
    if (attrs.empty() && zones.empty()) {
        return TBuffer();
    }

    const ui32 size = DirectMarkupByteSize(attrs, zones);
    TBuffer buf(size);
    ui8* p = reinterpret_cast<ui8*>(buf.Data());

    // repeated TAttribute Attributes = 1
    for (size_t i = 0; i < attrs.size(); ++i) {
        p = WriteRecord(1, ZoneAttrByteSize(attrs[i]), p);
        p = WriteZoneAttr(attrs[i], p);
    }
    // repeated TZone Zones = 2
    for (size_t i = 0; i < zones.size(); ++i) {
        p = WriteRecord(2, ZoneByteSize(zones[i]), p);
        p = WriteZone(zones[i], p);
    }

    Y_ASSERT((char*)p == buf.Data() + size);
    buf.Advance(size);
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace NIndexerCore
