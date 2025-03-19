#include "serialize.h"
#include <kernel/indexer/face/blob/directtext.pb.h>

#include <util/memory/pool.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite.h>

namespace NIndexerCore {

///////////////////////////////////////////////////////////////////////////////

using namespace google::protobuf::io;
using namespace google::protobuf::internal;

using TLimit = CodedInputStream::Limit;

///////////////////////////////////////////////////////////////////////////////

static bool ReadBytes(const char* base, CodedInputStream* input, TStringBuf* text) {
    ui32 textLen = 0;
    if (Y_UNLIKELY(!input->ReadVarint32(&textLen))) {
        return false;
    }
    // save pointer to the current string
    *text = TStringBuf(base + input->CurrentPosition(), textLen);
    // skip string
    if (!input->Skip(textLen)) {
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////

class TDirectTextHolder::TImpl : public TDirectTextData2 {
public:
    TImpl(const TStringBuf& data);

    const TDocInfoEx& GetDocInfo() const;

    const TDirectTextData2& GetDirectText() const;

    void FillDirectData(NIndexerCorePrivate::TDirectData* data, const TDirectTextData2* extra) const;

    void Process(IDirectTextCallback2& callback, IDocumentDataInserter* inserter,
        const TDirectTextData2* extraDirectText = nullptr) const;

private:
    bool Read         (const TStringBuf& data);
    bool ReadDocInfo  (const char* base, CodedInputStream* input);
    bool ReadCounts   (const char* base, CodedInputStream* input);
    bool ReadEntry    (const char* base, CodedInputStream* input, TDirectTextEntry2& e);
    bool ReadToken    (const char* base, CodedInputStream* input, TLemmatizedToken& token);
    bool ReadSpace    (const char* base, CodedInputStream* input, TDirectTextSpace& space);
    bool ReadZone     (const char* base, CodedInputStream* input, TDirectTextZone& zone);
    bool ReadAttrEntry(const char* base, CodedInputStream* input, TDirectAttrEntry& e);
    bool ReadZoneAttr (const char* base, CodedInputStream* input, TDirectTextZoneAttr& za);
    bool ReadZoneSpan (const char* base, CodedInputStream* input, TZoneSpan& span);
    bool ReadPosition (const char* base, CodedInputStream* input, ui16& sent, ui16& word);
    bool ReadSentAttr (const char* base, CodedInputStream* input, TDirectTextSentAttr& sa);

private:
    TFullArchiveDocHeader DocHeader_;
    TDocInfoEx DocInfo_;
    TMemoryPool Memory_;
};

///////////////////////////////////////////////////////////////////////////////

TDirectTextHolder::TImpl::TImpl(const TStringBuf& data)
    : DocHeader_()
    , DocInfo_()
    , Memory_(data.size())
{
    DocInfo_.DocHeader = &DocHeader_;

    if (!Read(data)) {
        ythrow yexception() << "can't read direct-text";
    }
}

const TDocInfoEx& TDirectTextHolder::TImpl::GetDocInfo() const {
    return DocInfo_;
}

const TDirectTextData2& TDirectTextHolder::TImpl::GetDirectText() const {
    return *this;
}

void TDirectTextHolder::TImpl::FillDirectData(NIndexerCorePrivate::TDirectData* data, const TDirectTextData2* extra) const {
    data->DirectText.Entries = this->Entries;
    data->DirectText.EntryCount = this->EntryCount;
    data->DirectText.SentAttrs = this->SentAttrs;
    data->DirectText.SentAttrCount = this->SentAttrCount;

    if (extra) {
        data->Zones.insert(data->Zones.end(), this->Zones,  this->Zones  + this->ZoneCount);
        data->Zones.insert(data->Zones.end(), extra->Zones, extra->Zones + extra->ZoneCount);

        data->ZoneAttrs.insert(data->ZoneAttrs.end(), this->ZoneAttrs,  this->ZoneAttrs  + this->ZoneAttrCount);
        data->ZoneAttrs.insert(data->ZoneAttrs.end(), extra->ZoneAttrs, extra->ZoneAttrs + extra->ZoneAttrCount);

        data->DirectText.Zones = data->Zones.data();
        data->DirectText.ZoneCount = data->Zones.size();
        data->DirectText.ZoneAttrs = data->ZoneAttrs.data();
        data->DirectText.ZoneAttrCount = data->ZoneAttrs.size();
    } else {
        data->DirectText.Zones = this->Zones;
        data->DirectText.ZoneCount = this->ZoneCount;
        data->DirectText.ZoneAttrs = this->ZoneAttrs;
        data->DirectText.ZoneAttrCount = this->ZoneAttrCount;
    }
}

void TDirectTextHolder::TImpl::Process(IDirectTextCallback2& callback, IDocumentDataInserter* inserter,
    const TDirectTextData2* extraDirectText) const
{
    callback.SetCurrentDoc(DocInfo_);

    if (extraDirectText) {
        NIndexerCorePrivate::TDirectData data;
        FillDirectData(&data, extraDirectText);
        callback.ProcessDirectText2(inserter, data.DirectText, DocInfo_.DocId);
    } else {
        callback.ProcessDirectText2(inserter, *this, DocInfo_.DocId);
    }
}

bool TDirectTextHolder::TImpl::Read(const TStringBuf& data) {
    CodedInputStream input(reinterpret_cast<const ui8*>(data.data()), data.size());
    ui32 tag;
    ui32 entryIdx = 0;
    ui32 zoneIdx = 0;
    ui32 zoneAttrIdx = 0;
    ui32 sentAttrIdx = 0;

    while ((tag = input.ReadTag()) != 0) {
        switch (WireFormatLite::GetTagFieldNumber(tag)) {
            case TDirectText::kDocInfoFieldNumber: {
                ui32 size = 0;
                if (Y_UNLIKELY(!input.ReadVarint32(&size))) {
                    return false;
                }
                TLimit limit = input.PushLimit(size);
                if (!ReadDocInfo(data.data(), &input)) {
                    return false;
                }
                input.PopLimit(limit);
                break;
            }

            case TDirectText::kCountsFieldNumber: {
                ui32 size = 0;
                if (Y_UNLIKELY(!input.ReadVarint32(&size))) {
                    return false;
                }
                TLimit limit = input.PushLimit(size);
                if (!ReadCounts(data.data(), &input)) {
                    return false;
                }
                input.PopLimit(limit);
                break;
            }

            case TDirectText::kEntriesFieldNumber: {
                ui32 size = 0;
                if (Y_UNLIKELY(!input.ReadVarint32(&size))) {
                    return false;
                }
                TLimit limit = input.PushLimit(size);
                if (!ReadEntry(data.data(), &input, const_cast<TDirectTextEntry2&>(Entries[entryIdx++]))) {
                    return false;
                }
                input.PopLimit(limit);
                break;
            }

            case TDirectText::kZonesFieldNumber: {
                ui32 size = 0;
                if (Y_UNLIKELY(!input.ReadVarint32(&size))) {
                    return false;
                }
                TLimit limit = input.PushLimit(size);
                if (!ReadZone(data.data(), &input, const_cast<TDirectTextZone&>(this->Zones[zoneIdx++]))) {
                    return false;
                }
                input.PopLimit(limit);
                break;
            }

            case TDirectText::kZoneAttrsFieldNumber: {
                ui32 size = 0;
                if (Y_UNLIKELY(!input.ReadVarint32(&size))) {
                    return false;
                }
                TLimit limit = input.PushLimit(size);
                if (!ReadZoneAttr(data.data(), &input, const_cast<TDirectTextZoneAttr&>(this->ZoneAttrs[zoneAttrIdx++]))) {
                    return false;
                }
                input.PopLimit(limit);
                break;
            }

            case TDirectText::kSentAttrsFieldNumber: {
                ui32 size = 0;
                if (Y_UNLIKELY(!input.ReadVarint32(&size))) {
                    return false;
                }
                TLimit limit = input.PushLimit(size);
                if (!ReadSentAttr(data.data(), &input, const_cast<TDirectTextSentAttr&>(this->SentAttrs[sentAttrIdx++]))) {
                    return false;
                }
                input.PopLimit(limit);
                break;
            }


            default: {
                if (Y_UNLIKELY(!WireFormatLite::SkipField(&input, tag))) {
                    return false;
                }
            }
        }
    }

    return    (input.CurrentPosition() == ssize_t(data.size()))
           && (entryIdx    == this->EntryCount)
           && (zoneIdx     == this->ZoneCount)
           && (zoneAttrIdx == this->ZoneAttrCount)
           && (sentAttrIdx == this->SentAttrCount);
}

bool TDirectTextHolder::TImpl::ReadDocInfo(const char* base, CodedInputStream* input) {
    while (const ui32 tag = input->ReadTag()) {
        switch (WireFormatLite::GetTagFieldNumber(tag)) {
            case TDirectText::TDocInfo::kDocIdFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &DocInfo_.DocId)) {
                    return false;
                }
                break;
            }

            case TDirectText::TDocInfo::kFeedIdFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<ui64, WireFormatLite::TYPE_UINT64>(input, &DocInfo_.FeedId)) {
                    return false;
                }
                break;
            }

            case TDirectText::TDocInfo::kUrlFlagsFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &DocInfo_.UrlFlags)) {
                    return false;
                }
                break;
            }

            case TDirectText::TDocInfo::kEncodingFieldNumber: {
                i32 value = 0;
                if (!WireFormatLite::ReadPrimitive<i32, WireFormatLite::TYPE_SINT32>(input, &value)) {
                    return false;
                } else {
                    DocHeader_.Encoding = value;
                }
                break;
            }

            case TDirectText::TDocInfo::kLanguageFieldNumber: {
                ui32 value = 0;
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &value)) {
                    return false;
                } else {
                    DocHeader_.Language = value;
                }
                break;
            }

            case TDirectText::TDocInfo::kLanguage2FieldNumber: {
                ui32 value = 0;
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &value)) {
                    return false;
                } else {
                    DocHeader_.Language2 = value;
                }
                break;
            }

            case TDirectText::TDocInfo::kIndexDateFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<i32, WireFormatLite::TYPE_SINT32>(input, &DocHeader_.IndexDate)) {
                    return false;
                }
                break;
            }

            case TDirectText::TDocInfo::kMimeTypeFieldNumber: {
                ui32 value = 0;
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &value)) {
                    return false;
                } else {
                    DocHeader_.MimeType = value;
                }
                break;
            }

            case TDirectText::TDocInfo::kUrlFieldNumber: {
                TStringBuf text;
                if (Y_UNLIKELY(!ReadBytes(base, input, &text))) {
                    return false;
                } else {
                    const size_t len = Min<size_t>(text.size(), URL_MAX - 1);
                    memcpy(DocHeader_.Url, text.data(), len);
                    DocHeader_.Url[len] = 0;
                }
                break;
            }

            default: {
                if (Y_UNLIKELY(!WireFormatLite::SkipField(input, tag))) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool TDirectTextHolder::TImpl::ReadCounts(const char*, CodedInputStream* input) {
    while (const ui32 tag = input->ReadTag()) {
        switch (WireFormatLite::GetTagFieldNumber(tag)) {
            case TDirectText::TCounts::kEntriesFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<ui64, WireFormatLite::TYPE_UINT64>(input, &this->EntryCount)) {
                    return false;
                }
                if (this->EntryCount) {
                    this->Entries = Memory_.NewArray<TDirectTextEntry2>(this->EntryCount);
                }
                break;
            }

            case TDirectText::TCounts::kZonesFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<ui64, WireFormatLite::TYPE_UINT64>(input, &this->ZoneCount)) {
                    return false;
                }
                if (this->ZoneCount) {
                    this->Zones = Memory_.NewArray<TDirectTextZone>(this->ZoneCount);
                }
                break;
            }

            case TDirectText::TCounts::kZoneAttrsFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<ui64, WireFormatLite::TYPE_UINT64>(input, &this->ZoneAttrCount)) {
                    return false;
                }
                if (this->ZoneAttrCount) {
                    this->ZoneAttrs = Memory_.NewArray<TDirectTextZoneAttr>(this->ZoneAttrCount);
                }
                break;
            }

            case TDirectText::TCounts::kSentAttrsFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<ui64, WireFormatLite::TYPE_UINT64>(input, &this->SentAttrCount)) {
                    return false;
                }
                if (this->SentAttrCount) {
                    this->SentAttrs = Memory_.NewArray<TDirectTextSentAttr>(this->SentAttrCount);
                }
                break;
            }

            default: {
                if (Y_UNLIKELY(!WireFormatLite::SkipField(input, tag))) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool TDirectTextHolder::TImpl::ReadEntry(const char* base, google::protobuf::io::CodedInputStream* input, TDirectTextEntry2& e) {
    ui32 tokenId = 0;
    ui32 spaceId = 0;
    ui32 tokenCount = 0;
    ui32 spaceCount = 0;

    while (const ui32 tag = input->ReadTag()) {
        switch (WireFormatLite::GetTagFieldNumber(tag)) {
            case TDirectText::TEntry::kPostingFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_FIXED32>(input, &e.Posting)) {
                    return false;
                }
                break;
            }

            case TDirectText::TEntry::kOffsetFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &e.OrigOffset)) {
                    return false;
                }
                break;
            }

            case TDirectText::TEntry::kOriginalTokenFieldNumber: {
                TStringBuf text;
                if (Y_UNLIKELY(!ReadBytes(base, input, &text))) {
                    return false;
                } else {
                    Y_ASSERT((text.size() % sizeof(wchar16)) == 0);
                    e.Token = TWtringBuf((const wchar16*)text.data(), (text.size() / sizeof(wchar16)) - 1);
                }
                break;
            }

            case TDirectText::TEntry::kTokensCountFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &tokenCount)) {
                    return false;
                } else {
                    e.LemmatizedToken      = Memory_.NewArray<TLemmatizedToken>(tokenCount);
                    e.LemmatizedTokenCount = tokenCount;
                }
                break;
            }

            case TDirectText::TEntry::kSpacesCountFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &spaceCount)) {
                    return false;
                } else {
                    e.Spaces     = Memory_.NewArray<TDirectTextSpace>(spaceCount);
                    e.SpaceCount = spaceCount;
                }
                break;
            }

            case TDirectText::TEntry::kTokensFieldNumber: {
                ui32 size;
                if (Y_UNLIKELY(!input->ReadVarint32(&size))) {
                    return false;
                }
                TLimit limit = input->PushLimit(size);
                if (!ReadToken(base, input, const_cast<TLemmatizedToken&>(e.LemmatizedToken[tokenId++]))) {
                    return false;
                }
                input->PopLimit(limit);
                break;
            }

            case TDirectText::TEntry::kSpacesFieldNumber: {
                ui32 size;
                if (Y_UNLIKELY(!input->ReadVarint32(&size))) {
                    return false;
                }
                TLimit limit = input->PushLimit(size);
                if (!ReadSpace(base, input, e.Spaces[spaceId++])) {
                    return false;
                }
                input->PopLimit(limit);
                break;
            }

            default: {
                if (Y_UNLIKELY(!WireFormatLite::SkipField(input, tag))) {
                    return false;
                }
            }
        }
    }
    return (spaceId == spaceCount) &&
           (tokenId == tokenCount);
}

bool TDirectTextHolder::TImpl::ReadToken(const char* base, CodedInputStream* input, TLemmatizedToken& token) {
    ui32 gramsCount = 0;
    ui32 gramsId = 0;

    while (const ui32 tag = input->ReadTag()) {
        switch (WireFormatLite::GetTagFieldNumber(tag)) {
            case TDirectText::TToken::kLangFieldNumber: {
                ui32 value = 0;
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &value)) {
                    return false;
                } else {
                    token.Lang = value;
                }
                break;
            }

            case TDirectText::TToken::kFlagsFieldNumber: {
                ui32 value = 0;
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &value)) {
                    return false;
                } else {
                    token.Flags = value;
                }
                break;
            }

            case TDirectText::TToken::kJoinsFieldNumber: {
                ui32 value = 0;
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &value)) {
                    return false;
                } else {
                    token.Joins = value;
                }
                break;
            }

            case TDirectText::TToken::kFormOffsetFieldNumber: {
                ui32 value = 0;
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &value)) {
                    return false;
                } else {
                    token.FormOffset = value;
                }
                break;
            }

            case TDirectText::TToken::kTermCountFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<i32, WireFormatLite::TYPE_SINT32>(input, &token.TermCount)) {
                    return false;
                }
                break;
            }

            case TDirectText::TToken::kWeightFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<double, WireFormatLite::TYPE_DOUBLE>(input, &token.Weight)) {
                    return false;
                }
                break;
            }

            case TDirectText::TToken::kStemGramFieldNumber: {
                TStringBuf text;
                if (Y_UNLIKELY(!ReadBytes(base, input, &text))) {
                    return false;
                } else {
                    token.StemGram = text.data();
                }
                break;
            }

            case TDirectText::TToken::kFlexGramsCountFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &gramsCount)) {
                    return false;
                } else {
                    token.FlexGrams = Memory_.NewArray<const char*>(gramsCount);
                    token.GramCount = gramsCount;
                }

                break;
            }

            case TDirectText::TToken::kFlexGramsFieldNumber: {
                TStringBuf text;
                if (Y_UNLIKELY(!ReadBytes(base, input, &text))) {
                    return false;
                } else {
                    const_cast<const char*&>(token.FlexGrams[gramsId++]) = text.data();
                }
                break;
            }

            case TDirectText::TToken::kLemmaTextFieldNumber: {
                TStringBuf text;
                if (Y_UNLIKELY(!ReadBytes(base, input, &text))) {
                    return false;
                } else {
                    token.LemmaText = text.data();
                }
                break;
            }

            case TDirectText::TToken::kFormaTextFieldNumber: {
                TStringBuf text;
                if (Y_UNLIKELY(!ReadBytes(base, input, &text))) {
                    return false;
                } else {
                    token.FormaText = text.data();
                }
                break;
            }

            case TDirectText::TToken::kIsBastardFieldNumber: {
                ui32 value = 0;
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &value)) {
                    return false;
                } else {
                    token.IsBastard = value;
                }
                break;
            }

            case TDirectText::TToken::kPrefixFieldNumber: {
                ui32 value = 0;
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &value)) {
                    return false;
                } else {
                    token.Prefix = value;
                }
                break;
            }

            default: {
                if (Y_UNLIKELY(!WireFormatLite::SkipField(input, tag))) {
                    return false;
                }
            }
        }
    }
    return (gramsCount == gramsId);
}

bool TDirectTextHolder::TImpl::ReadSpace(const char* base, CodedInputStream* input, TDirectTextSpace& space) {
    while (const ui32 tag = input->ReadTag()) {
        switch (WireFormatLite::GetTagFieldNumber(tag)) {
            case TDirectText::TSpace::kSpaceFieldNumber: {
                TStringBuf text;
                if (Y_UNLIKELY(!ReadBytes(base, input, &text))) {
                    return false;
                } else {
                    Y_ASSERT((text.size() % sizeof(wchar16)) == 0);
                    space.Space  = (const wchar16*)text.data();
                    space.Length = text.size() / sizeof(wchar16);
                }
                break;
            }

            case TDirectText::TSpace::kBreakTypeFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &space.Type)) {
                    return false;
                }
                break;
            }

            default: {
                if (Y_UNLIKELY(!WireFormatLite::SkipField(input, tag))) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool TDirectTextHolder::TImpl::ReadZone(const char* base, CodedInputStream* input, TDirectTextZone& zone) {
    TVector<TZoneSpan> spans;

    while (const ui32 tag = input->ReadTag()) {
        switch (WireFormatLite::GetTagFieldNumber(tag)) {
            case TDirectMarkup::TZone::kTypeFieldNumber: {
                ui32 value = 0;
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &value)) {
                    return false;
                } else {
                    zone.ZoneType = value;
                }
                break;
            }

            case TDirectMarkup::TZone::kNameFieldNumber: {
                TStringBuf text;
                if (Y_UNLIKELY(!ReadBytes(base, input, &text))) {
                    return false;
                } else {
                    zone.Zone = TStringBuf(text.data(), text.size() - 1);
                }
                break;
            }

            case TDirectMarkup::TZone::kSpansFieldNumber: {
                ui32 size = 0;
                if (Y_UNLIKELY(!input->ReadVarint32(&size))) {
                    return false;
                }
                TLimit limit = input->PushLimit(size);
                TZoneSpan span;
                if (!ReadZoneSpan(base, input, span)) {
                    return false;
                } else {
                    spans.push_back(span);
                }
                input->PopLimit(limit);
                break;
            }

            default: {
                if (Y_UNLIKELY(!WireFormatLite::SkipField(input, tag))) {
                    return false;
                }
            }
        }
    }

    if (!spans.empty()) {
        zone.Spans     = Memory_.NewArray<TZoneSpan>(spans.size());
        zone.SpanCount = spans.size();

        for (size_t i = 0; i < spans.size(); ++i) {
            const_cast<TZoneSpan&>(zone.Spans[i]) = spans[i];
        }
    }

    return true;

}

bool TDirectTextHolder::TImpl::ReadPosition(const char*, CodedInputStream* input, ui16& sent, ui16& word) {
    while (const ui32 tag = input->ReadTag()) {
        switch (WireFormatLite::GetTagFieldNumber(tag)) {
            case TDirectMarkup::TPosition::kSentFieldNumber: {
                ui32 value = 0;
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &value)) {
                    return false;
                } else {
                    sent = value;
                }
                break;
            }

            case TDirectMarkup::TPosition::kWordFieldNumber: {
                ui32 value = 0;
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &value)) {
                    return false;
                } else {
                    word = value;
                }
                break;
            }

            default: {
                if (Y_UNLIKELY(!WireFormatLite::SkipField(input, tag))) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool TDirectTextHolder::TImpl::ReadAttrEntry(const char* base, CodedInputStream* input, TDirectAttrEntry& e) {
    while (const ui32 tag = input->ReadTag()) {
        switch (WireFormatLite::GetTagFieldNumber(tag)) {
            case TDirectMarkup::TAttrEntry::kPositionFieldNumber: {
                ui32 size = 0;
                if (Y_UNLIKELY(!input->ReadVarint32(&size))) {
                    return false;
                }
                TLimit limit = input->PushLimit(size);
                if (!ReadPosition(base, input, e.Sent, e.Word)) {
                    return false;
                }
                input->PopLimit(limit);
                break;
            }

            case TDirectMarkup::TAttrEntry::kValueFieldNumber: {
                TStringBuf text;
                if (Y_UNLIKELY(!ReadBytes(base, input, &text))) {
                    return false;
                } else {
                    Y_ASSERT((text.size() % sizeof(wchar16)) == 0);
                    e.AttrValue = TWtringBuf((const wchar16*)text.data(), (text.size() / sizeof(wchar16)) - 1);
                }
                break;
            }

            case TDirectMarkup::TAttrEntry::kNoFollowFieldNumber: {
                if (!WireFormatLite::ReadPrimitive<bool, WireFormatLite::TYPE_BOOL>(input, &e.NoFollow)) {
                    return false;
                }
                break;
            }

            default: {
                if (Y_UNLIKELY(!WireFormatLite::SkipField(input, tag))) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool TDirectTextHolder::TImpl::ReadZoneAttr(const char* base, CodedInputStream* input, TDirectTextZoneAttr& za) {
    TVector<TDirectAttrEntry> entries;

    while (const ui32 tag = input->ReadTag()) {
        switch (WireFormatLite::GetTagFieldNumber(tag)) {
            case TDirectMarkup::TAttribute::kTypeFieldNumber: {
                ui32 value = 0;
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &value)) {
                    return false;
                } else {
                    za.AttrType = value;
                }
                break;
            }

            case TDirectMarkup::TAttribute::kNameFieldNumber: {
                TStringBuf text;
                if (Y_UNLIKELY(!ReadBytes(base, input, &text))) {
                    return false;
                } else {
                    za.AttrName = TStringBuf(text.data(), text.size() - 1);
                }
                break;
            }

            case TDirectMarkup::TAttribute::kEntriesFieldNumber: {
                ui32 size = 0;
                if (Y_UNLIKELY(!input->ReadVarint32(&size))) {
                    return false;
                }
                TLimit limit = input->PushLimit(size);
                TDirectAttrEntry e;
                if (!ReadAttrEntry(base, input, e)) {
                    return false;
                } else {
                    entries.push_back(e);
                }
                input->PopLimit(limit);
                break;
            }

            default: {
                if (Y_UNLIKELY(!WireFormatLite::SkipField(input, tag))) {
                    return false;
                }
            }
        }
    }

    if (!entries.empty()) {
        za.Entries    = Memory_.NewArray<TDirectAttrEntry>(entries.size());
        za.EntryCount = entries.size();

        for (size_t i = 0; i < entries.size(); ++i) {
            const_cast<TDirectAttrEntry&>(za.Entries[i]) = entries[i];
        }
    }

    return true;
}

bool TDirectTextHolder::TImpl::ReadZoneSpan(const char* base, CodedInputStream* input, TZoneSpan& span) {
    while (const ui32 tag = input->ReadTag()) {
        switch (WireFormatLite::GetTagFieldNumber(tag)) {
            case TDirectMarkup::TZoneSpan::kBeginFieldNumber: {
                ui32 size = 0;
                if (Y_UNLIKELY(!input->ReadVarint32(&size))) {
                    return false;
                }
                TLimit limit = input->PushLimit(size);
                if (!ReadPosition(base, input, span.SentBeg, span.WordBeg)) {
                    return false;
                }
                input->PopLimit(limit);
                break;
            }

            case TDirectMarkup::TZoneSpan::kEndFieldNumber: {
                ui32 size = 0;
                if (Y_UNLIKELY(!input->ReadVarint32(&size))) {
                    return false;
                }
                TLimit limit = input->PushLimit(size);
                if (!ReadPosition(base, input, span.SentEnd, span.WordEnd)) {
                    return false;
                }
                input->PopLimit(limit);
                break;
            }

            default: {
                if (Y_UNLIKELY(!WireFormatLite::SkipField(input, tag))) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool TDirectTextHolder::TImpl::ReadSentAttr(const char* base, CodedInputStream* input, TDirectTextSentAttr& sa) {
    while (const ui32 tag = input->ReadTag()) {
        switch (WireFormatLite::GetTagFieldNumber(tag)) {
            case TDirectText::TSentAttr::kSentFieldNumber: {
                ui32 value = 0;
                if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &value)) {
                    return false;
                } else {
                    sa.Sent = value;
                }
                break;
            }

            case TDirectText::TSentAttr::kNameFieldNumber: {
                TStringBuf text;
                if (Y_UNLIKELY(!ReadBytes(base, input, &text))) {
                    return false;
                } else {
                    sa.Attr = TString(text);
                }
                break;
            }

            case TDirectText::TSentAttr::kValueFieldNumber: {
                TStringBuf text;
                if (Y_UNLIKELY(!ReadBytes(base, input, &text))) {
                    return false;
                } else {
                    sa.Value = TString(text);
                }
                break;
            }

            default: {
                if (Y_UNLIKELY(!WireFormatLite::SkipField(input, tag))) {
                    return false;
                }
            }
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////

TDirectTextHolder::TDirectTextHolder(const TDirectTextRef& data)
    : Impl_(new TImpl(data))
{
}

TDirectTextHolder::~TDirectTextHolder() {
}

const TDocInfoEx& TDirectTextHolder::GetDocInfo() const {
    return Impl_->GetDocInfo();
}

const TDirectTextData2& TDirectTextHolder::GetDirectText() const {
    return Impl_->GetDirectText();
}

void TDirectTextHolder::FillDirectData(NIndexerCorePrivate::TDirectData* data, const TDirectTextData2* extra) const {
    Impl_->FillDirectData(data, extra);
}

void TDirectTextHolder::Process(IDirectTextCallback2& callback, IDocumentDataInserter* inserter,
    const TDirectTextData2* extraDirectText) const
{
    Impl_->Process(callback, inserter, extraDirectText);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace NIndexerCore
