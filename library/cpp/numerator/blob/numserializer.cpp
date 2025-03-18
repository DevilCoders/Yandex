#include "numserializer.h"

#include <library/cpp/html/face/zonecount.h>
#include <library/cpp/html/zoneconf/ht_conf.h>
#include <library/cpp/html/zoneconf/attrextractor.h>
#include <library/cpp/packedtypes/longs.h>

#include <util/generic/cast.h>

namespace {
    const ui8 ONTOKENSTART = 1;
    const ui8 ONSPACES = 2;

    const ui8 ONTokenPos = 4;
    const ui8 ONWordCount = 5;
    const ui8 ONCurWeight = 6;
    const ui8 ONCurIrregTag = 7;
    const ui8 ONInputPosition = 8;
    const ui8 ONPosOK = 9;

    const ui8 ONTEXTSTART = 10;
    const ui8 ONTEXTEND = 11;
    const ui8 ONPARAGRAPHEND = 12;
    const ui8 ONADDEVENT = 13;
    const ui8 ONREMOVEEVENT = 14;
    const ui8 ONTOKENBEGIN = 15;
    const ui8 ONTOKENEND = 16;
    const ui8 ONMOVEINPUT = 17;

    // zone info bits
    const ui8 ZONE_HAS_NAME = 1;
    const ui8 ZONE_IS_OPEN = 2;
    const ui8 ZONE_IS_CLOSE = 4;
    const ui8 ZONE_ONLY_ATTRS = 8;
    const ui8 ZONE_NO_OPENING_TAG = 16;
    const ui8 ZONE_HAS_SPONSORED_ATTR = 32;
    const ui8 ZONE_HAS_UGC_ATTR = 64;

#pragma pack(1)
    struct Y_PACKED TCharSpanPacked {
        ui8 Pos;
        ui8 Len;
        ui8 PrefixLen : 4;
        ui8 SuffixLen : 4;
        ui8 Type : 3;
        ui8 Hyphen : 2;
        ui8 TokenDelim : 3;
    };
#pragma pack()
    static_assert(sizeof(TCharSpanPacked) == 4, "expect sizeof(TCharSpanPacked) == 4");

    inline i32 ReadInt(const ui8*& ptr) {
        i32 ret;
        ptr += in_long(ret, (const char*)ptr);
        return ret;
    }

    inline void WriteInt(const i32 val, TBuffer* res) {
        const int len = len_long(val);
        res->Resize(res->Size() + len);
        out_long(val, res->Pos() - len);
    }

    inline void Pack(const TWideToken& wt, TBuffer* res) {
        ui8 size = IntegerCast<ui8>(wt.Leng);
        res->Append((const char*)&size, sizeof(size));
        res->Append((const char*)wt.Token, sizeof(wchar16) * size);
        const TTokenStructure& ts = wt.SubTokens;
        size = IntegerCast<ui8>(ts.size());
        res->Append((const char*)&size, sizeof(size));
        for (ui8 i = 0; i < size; i++) {
            const TCharSpan& cs = ts[i];
            TCharSpanPacked csp;
            csp.Pos = IntegerCast<ui8>(cs.Pos);
            csp.Len = IntegerCast<ui8>(cs.Len);
            csp.PrefixLen = IntegerCast<ui8>(cs.PrefixLen);
            csp.SuffixLen = IntegerCast<ui8>(cs.SuffixLen);
            csp.Type = cs.Type;
            csp.Hyphen = cs.Hyphen;
            csp.TokenDelim = cs.TokenDelim;
            res->Append((const char*)&csp, sizeof(csp));
        }
    }

    inline const ui8* Unpack(const ui8* ptr, TWideToken* res) {
        res->Leng = *ptr++;
        res->Token = (wchar16*)ptr;
        ptr += sizeof(wchar16) * res->Leng;
        ui8 size = *ptr++;
        for (ui8 i = 0; i < size; i++) {
            const TCharSpanPacked* csp = (const TCharSpanPacked*)ptr;
            ptr += sizeof(TCharSpanPacked);
            res->SubTokens.push_back(csp->Pos, csp->Len, (ETokenType)csp->Type, (ETokenDelim)csp->TokenDelim, (EHyphenType)csp->Hyphen, csp->SuffixLen, csp->PrefixLen);
        }
        return ptr;
    }

    inline void SerializeText(TBuffer* out, const wchar16* text, size_t len) {
        const ui32 n32 = IntegerCast<ui32>(len);
        out->Append((const char*)&n32, sizeof(n32));
        const size_t n = len + 1;                       // store with null-terminator
        out->Resize(out->Size() + sizeof(wchar16) * n); // buffer can be reallocated
        wchar16* p = (wchar16*)out->Pos() - (ptrdiff_t)n;
        memcpy(p, text, sizeof(wchar16) * len);
        p[len] = 0;
    }

    inline void AppendAttributeString(TBuffer* out, const TSerializedString& str) {
        const size_t size = str.size() + 1;
        out->Append(str.c_str(), size);
        // Check for null bytes in string
        char* p = out->Pos() - ptrdiff_t(size);
        char* ed = p + str.size();
        for (; p != ed; ++p) {
            if (*p == 0) {
                *p = ' ';
            }
        }
    }

    inline void SerializeZoneInfo(TBuffer* out, const TZoneEntry* zone) {
        // zone is NULL if it has no attrs, !IsOpen and !IsClose, in this case info is equal to 0
        if (!zone) {
            out->Append(0); // info == 0
            out->Append(0); // +attrs == 0
            return;
        }
        Y_ASSERT(zone->IsValid());
        const ui8 info = (zone->Name ? ZONE_HAS_NAME : 0) | (zone->OnlyAttrs ? ZONE_ONLY_ATTRS : 0) | (zone->IsOpen ? ZONE_IS_OPEN : 0) | (zone->IsClose ? ZONE_IS_CLOSE : 0) | (zone->NoOpeningTag ? ZONE_NO_OPENING_TAG : 0) |
                         (zone->HasSponsoredAttr ? ZONE_HAS_SPONSORED_ATTR : 0) | (zone->HasUserGeneratedContentAttr ? ZONE_HAS_UGC_ATTR : 0);
        out->Append(info);
        if (zone->Name)
            out->Append(zone->Name, strlen(zone->Name) + 1);
        if (zone->Attrs.size() > 255)
            ythrow yexception() << "too many attributes in zone";
        out->Append((char)zone->Attrs.size()); // max 255
        for (size_t i = 0; i < zone->Attrs.size(); ++i) {
            const TAttrEntry& a = zone->Attrs[i];
            AppendAttributeString(out, a.Name);
            AppendAttributeString(out, a.Value);
            SerializeText(out, a.DecodedValue.data(), a.DecodedValue.size());
            out->Append((char)a.Type);
            out->Append((char)a.Pos);
        }
    }

    inline const ui8* DeserializeZoneInfo(const ui8* ptr, const ui8* end, TZoneEntry* zone) {
        Y_VERIFY(ptr + sizeof(ui16) <= end, "invalid zone buffer");
        const ui8 info = *ptr++;
        if (info & ZONE_HAS_NAME) {
            zone->Name = (const char*)ptr;
            ptr += strlen(zone->Name) + 1;
        }
        zone->IsOpen = (info & ZONE_IS_OPEN) != 0;
        zone->IsClose = (info & ZONE_IS_CLOSE) != 0;
        zone->OnlyAttrs = (info & ZONE_ONLY_ATTRS) != 0;
        zone->NoOpeningTag = (info & ZONE_NO_OPENING_TAG) != 0;
        zone->HasSponsoredAttr = (info & ZONE_HAS_SPONSORED_ATTR) != 0;
        zone->HasUserGeneratedContentAttr = (info & ZONE_HAS_UGC_ATTR) != 0;
        const ui8 n = *ptr++;
        for (ui8 i = 0; i < n; ++i) {
            const TStringBuf name((const char*)ptr);
            ptr += name.size() + 1;
            const TStringBuf value((const char*)ptr);
            ptr += value.size() + 1;
            ui32 len = *((ui32*)ptr);
            ptr += sizeof(ui32);
            const wchar16* decValue = (const wchar16*)ptr;
            ptr += (len + 1) * sizeof(wchar16); // value null-terminated
            ATTR_TYPE type = (ATTR_TYPE)*ptr++;
            ATTR_POS pos = (ATTR_POS)*ptr++;
            TAttrEntry attr(name.data(), name.size(), value.data(), value.size(), type, pos);
            attr.DecodedValue = TWtringBuf(decValue, len);
            Y_ASSERT(decValue[len] == 0); // it must be null-terminated
            zone->Attrs.push_back(attr);
        }
        return ptr;
    }

    template <typename T>
    inline void Save(TBuffer* out, ui8 eventType, T value) {
        out->Append((const char*)&eventType, sizeof(ui8));
        out->Append((const char*)&value, sizeof(value));
    }

    template <>
    inline void Save(TBuffer* out, ui8 eventType, TEXT_WEIGHT value) {
        out->Append((const char*)&eventType, sizeof(ui8));
        ui8 w = (ui8)value;
        out->Append((const char*)&w, sizeof(w));
    }

    template <>
    inline void Save(TBuffer* out, ui8 eventType, TIrregTag value) {
        out->Append((const char*)&eventType, sizeof(ui8));
        ui32 w = (ui32)value;
        out->Append((const char*)&w, sizeof(w));
    }

    template <>
    inline void Save(TBuffer* out, ui8 eventType, bool posOK) {
        if (!posOK)
            out->Append((const char*)&eventType, sizeof(ui8));
    }

    template <typename T>
    inline void SaveIf(TBuffer* out, ui8 eventType, T* oldValue, T newValue) {
        if (newValue != *oldValue) {
            *oldValue = newValue;
            Save(out, eventType, newValue);
        }
    }

    inline const THtmlChunk& GetChunkRef(const TVector<const THtmlChunk*>& chunks, const ui8*& ptr) {
        return *chunks.at(ReadInt(ptr));
    }

}

class TNumerSerializer::TStatChecker {
public:
    TStatChecker(const TNumerStat& stat, TBuffer* out)
        : LastStat(stat)
        , Out(out)
    {
        Save(Out, ONTokenPos, LastStat.TokenPos.Pos);
        Save(Out, ONWordCount, LastStat.WordCount);
        Save(Out, ONCurWeight, LastStat.CurWeight);
        Save(Out, ONCurIrregTag, LastStat.CurIrregTag);
        Save(Out, ONInputPosition, LastStat.InputPosition);
        Save(Out, ONPosOK, LastStat.PosOK);
    }

    void Check(const TNumerStat& stat) {
        SaveIf(Out, ONTokenPos, &LastStat.TokenPos.Pos, stat.TokenPos.Pos);
        SaveIf(Out, ONWordCount, &LastStat.WordCount, stat.WordCount);
        SaveIf(Out, ONCurWeight, &LastStat.CurWeight, stat.CurWeight);
        SaveIf(Out, ONCurIrregTag, &LastStat.CurIrregTag, stat.CurIrregTag);
        SaveIf(Out, ONInputPosition, &LastStat.InputPosition, stat.InputPosition);
        SaveIf(Out, ONPosOK, &LastStat.PosOK, stat.PosOK);
    }

private:
    TNumerStat LastStat;
    TBuffer* const Out;
};

class TNumerSerializer::TZoneExtractor {
public:
    // props    actually every conf can add properties to docProps so
    //          there are needed several docProps or htparser.ini-files must NOT
    //          contain the same attributes that are added to docProps
    TZoneExtractor(IParsedDocProperties* props, const THtConfigurator* conf)
        : Extractor_(conf, false)
        , Counter_(&Extractor_, props)
    {
    }

    inline void CheckEvent(const THtmlChunk& e, TZoneEntry* zone) {
        Counter_.CheckEvent(e, zone);
    }

    inline const HashSet& GetOpenZones() const {
        return Counter_.GetOpenZones();
    }

private:
    TAttributeExtractor Extractor_;
    TZoneCounter Counter_;
};

TNumerSerializer::TNumerSerializer(TBuffer& outBuffer, IParsedDocProperties* props,
                                   const THtConfigurator* configs, size_t configCount, TBuffer* zoneBuffers)
    : Out_(&outBuffer)
    , ZoneBuffers_(zoneBuffers)
    , StatChecker_(new TStatChecker(TNumerStat(), Out_)) // empty numerator status - since numerator is created on every document
    , Decoder_(props->GetCharset())
    , BaseIdx_(-1)
    , LastIdx_(-1)
{
    for (size_t i = 0; i < configCount; ++i) {
        Extractors_.push_back(new TZoneExtractor(props, &configs[i]));
    }
}

TNumerSerializer::~TNumerSerializer() {
}

void TNumerSerializer::OnTextStart(const IParsedDocProperties*) {
    Out_->Append((const char*)&ONTEXTSTART, sizeof(ui8));
}

void TNumerSerializer::OnTextEnd(const IParsedDocProperties*, const TNumerStat& stat) {
    StatChecker_->Check(stat);
    Out_->Append((const char*)&ONTEXTEND, sizeof(ui8));
}

void TNumerSerializer::OnParagraphEnd() {
    Out_->Append((const char*)&ONPARAGRAPHEND, sizeof(ui8));
}

void TNumerSerializer::OnSpaces(TBreakType bt, const wchar16* token, unsigned len, const TNumerStat& stat) {
    Y_VERIFY(!(bt & (ST_ZONEOPN | ST_ZONECLS)), "break type incorrect"); // replace with Y_ASSERT

    // type must have no ST_ZONEOPN and ST_ZONECLS bits, nextSpaceType is added during deserialization
    Y_ASSERT((bt & (ST_ZONEOPN | ST_ZONECLS)) == 0);
    StatChecker_->Check(stat);
    Out_->Append((const char*)&ONSPACES, sizeof(ui8));
    ui8 bt1 = (ui8)bt;
    Out_->Append((const char*)&bt1, sizeof(bt1));
    ui32 len2 = IntegerCast<ui32>(len);
    Out_->Append((const char*)&len2, sizeof(len2));
    Out_->Append((const char*)token, sizeof(wchar16) * len2);
}

void TNumerSerializer::OnAddEvent(const THtmlChunk& e) {
    AppendToBuffer(ONADDEVENT, &e);
}

void TNumerSerializer::OnRemoveEvent(const THtmlChunk& e) {
    AppendToBuffer(ONREMOVEEVENT, &e);
}

void TNumerSerializer::OnTokenBegin(const THtmlChunk& e, unsigned) {
    AppendToBuffer(ONTOKENBEGIN, &e);
}

void TNumerSerializer::OnTokenEnd(const THtmlChunk& e, unsigned) {
    AppendToBuffer(ONTOKENEND, &e);
}

void TNumerSerializer::OnTokenStart(const TWideToken& wt, const TNumerStat& stat) {
    StatChecker_->Check(stat);
    Out_->Append((const char*)&ONTOKENSTART, sizeof(ui8));
    Pack(wt, Out_);
}

void TNumerSerializer::OnMoveInput(const THtmlChunk& chunk, const TZoneEntry* zoneNull, const TNumerStat& stat) {
    Y_VERIFY(!zoneNull, "zone must be NULL"); // replace with Y_ASSERT
    StoreChunkAndStat(ONMOVEINPUT, chunk, stat);

    const int type = chunk.flags.type;
    for (size_t i = 0; i < Extractors_.size(); ++i) {
        if (type == PARSED_EOF) {
            const HashSet& openZones = Extractors_[i]->GetOpenZones();
            // parser must insert missing tags...
            if (!openZones.empty())
                ythrow yexception() << "Zone was not closed: '" << openZones.begin()->first << "'";
        } else {
            TZoneEntry zone;
            Extractors_[i]->CheckEvent(chunk, &zone);

            if (zone.IsValid()) {
                if (zone.Attrs.size()) {
                    TCharTemp decodeBuf(GetDecodeBufferSize(zone.Attrs.data(), zone.Attrs.size(), &Decoder_));
                    DecodeAttrValues(zone.Attrs.data(), zone.Attrs.size(), decodeBuf.Data(), &Decoder_);
                    SerializeZoneInfo(&ZoneBuffers_[i], &zone);
                } else {
                    SerializeZoneInfo(&ZoneBuffers_[i], &zone);
                }
            } else {
                SerializeZoneInfo(&ZoneBuffers_[i], nullptr);
            }
        }
    }
}

void TNumerSerializer::AppendToBuffer(ui8 type, const THtmlChunk* chunk) {
    const ui32 idx = NHtml::GetHtmlChunkIndex(chunk);

    if (type == ONADDEVENT) {
        if (BaseIdx_ == -1) {
            BaseIdx_ = idx;
        }

        LastIdx_ = idx;

        Out_->Append((const char*)&type, sizeof(type));
    } else if (type == ONREMOVEEVENT) {
        if (int(idx) == LastIdx_) {
            BaseIdx_ = -1;
            LastIdx_ = -1;

            Out_->Append((const char*)&type, sizeof(type));
        }
    } else {
        Y_ASSERT(BaseIdx_ != -1);
        Y_ASSERT(int(idx) >= BaseIdx_ && int(idx) <= LastIdx_);

        const i32 pos = idx - BaseIdx_;
        Out_->Append((const char*)&type, sizeof(type));
        WriteInt(pos, Out_);
    }
}

void TNumerSerializer::StoreChunkAndStat(ui8 type, const THtmlChunk& chunk, const TNumerStat& stat) {
    StatChecker_->Check(stat);
    AppendToBuffer(type, &chunk);
}

void Deserialize(const void* inputData, size_t inputSize, INumeratorHandler* handler, const NHtml::TStorage& storage,
                 const IParsedDocProperties* parser, const char* zoneData, size_t zoneDataSize) {
    TNumerStat stat;
    TZoneEntry zone;
    const ui8* ptr = (const ui8*)inputData;
    const ui8* const end = (const ui8*)inputData + inputSize;
    const ui8* zonePtr = (const ui8*)zoneData;
    const ui8* zoneEnd = (const ui8*)zoneData + zoneDataSize;
    int nextSpaceType = ST_NOBRK;

    auto si = storage.Begin();
    TVector<const THtmlChunk*> chunks;

    while (ptr < end) {
        const ui8 type = *ptr++;
        switch (type) {
            case ONTokenPos: {
                stat.TokenPos.Pos = *((SUPERLONG*)ptr);
                ptr += sizeof(SUPERLONG);
                break;
            }
            case ONWordCount: {
                stat.WordCount = *((ui32*)ptr);
                ptr += sizeof(ui32);
                break;
            }
            case ONCurWeight: {
                stat.CurWeight = TEXT_WEIGHT(*reinterpret_cast<const i8*>(ptr++));
                break;
            }
            case ONCurIrregTag: {
                stat.CurIrregTag = (TIrregTag) * ((ui32*)ptr);
                ptr += sizeof(ui32);
                break;
            }
            case ONInputPosition: {
                stat.InputPosition = *((ui32*)ptr);
                ptr += sizeof(ui32);
                break;
            }
            case ONPosOK: {
                stat.PosOK = false;
                break;
            }
            case ONTOKENSTART: {
                TWideToken tok;
                ptr = Unpack(ptr, &tok);
                handler->OnTokenStart(tok, stat);
                nextSpaceType = ST_NOBRK;
                break;
            }
            case ONSPACES: {
                ui8 bt = *ptr++;
                ui32 len = *((ui32*)ptr);
                ptr += sizeof(ui32);
                if (len) {
                    handler->OnSpaces((TBreakType)bt | nextSpaceType, (const wchar16*)ptr, len, stat);
                    nextSpaceType = ST_NOBRK;
                } else {
                    // don't add nextSpaceType to PARA/SENT breaks, see call to OnSpaces() in Numerator::ProcessEvent()
                    handler->OnSpaces((TBreakType)bt, (const wchar16*)ptr, len, stat);
                }
                ptr += len * sizeof(wchar16);
                break;
            }
            case ONTEXTSTART:
                handler->OnTextStart(parser);
                break;
            case ONTEXTEND:
                handler->OnTextEnd(parser, stat);
                break;
            case ONPARAGRAPHEND:
                handler->OnParagraphEnd();
                break;

            case ONADDEVENT: {
                const THtmlChunk* chunk = NHtml::GetHtmlChunk(si);
                ++si;
                chunks.push_back(chunk);
                handler->OnAddEvent(*chunk);
                break;
            }
            case ONREMOVEEVENT:
                for (auto& chunk : chunks) {
                    handler->OnRemoveEvent(*chunk);
                }
                chunks.clear();
                break;
            case ONTOKENBEGIN:
                handler->OnTokenBegin(GetChunkRef(chunks, ptr), 0);
                break;
            case ONTOKENEND:
                handler->OnTokenEnd(GetChunkRef(chunks, ptr), 0);
                break;
            case ONMOVEINPUT: {
                const THtmlChunk& chunk = GetChunkRef(chunks, ptr);
                if (zonePtr) {
                    if (chunk.flags.type == PARSED_EOF)
                        Y_VERIFY(zonePtr == zoneEnd, "invalid zone data");
                    else
                        zonePtr = DeserializeZoneInfo(zonePtr, zoneEnd, &zone);
                }
                if (zone.IsValid()) {
                    if (zone.IsOpen && !zone.OnlyAttrs)
                        nextSpaceType = ST_ZONEOPN;

                    if (zone.IsClose && !zone.NoOpeningTag)
                        nextSpaceType = ST_ZONECLS;

                    handler->OnMoveInput(chunk, &zone, stat);
                    zone.Reset();
                } else
                    handler->OnMoveInput(chunk, nullptr, stat);
                break;
            }
            default:
                ythrow yexception() << "unknown format (2)";
        }
    }
    Y_ASSERT(ptr == end);
}
