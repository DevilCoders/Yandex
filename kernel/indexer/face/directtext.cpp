#include "directtext.h"

namespace NIndexerCore {
namespace NIndexerCorePrivate {

TDirectZones::TDirectZones()
{ }

TDirectZones::~TDirectZones()
{ }

size_t TDirectZones::Capacity() const{
    return 1000;
}

void TDirectZones::Clear() {
    Zones.clear();
    Attrs.clear();
    ZoneMap.clear();
    AttrMap.clear();
}

void TDirectZones::StoreZone(ui8 ztype, const TStringBuf& key, TPosting pos) {
    char otype = key[0];
    ui16 wd = GetWord(pos);
    ui16 br = GetBreak(pos);
    TVector<TZoneSpan>& zs = InsertOrGetZone(TString(key.SubStr(1)), ztype);

    if (otype == '(') {
        // skip beginning of zone that is inside of the previous zone
        if (zs.empty() || (zs.back().SentEnd < br || (zs.back().SentEnd == br && zs.back().WordEnd <= wd))) { // start the next zone
            zs.resize(zs.size() + 1);
            zs.back().SentBeg = br;
            zs.back().WordBeg = wd;
        } else {
            Y_ASSERT(zs.back().SentBeg < br || (zs.back().SentBeg == br && zs.back().WordBeg <= wd));
        }
    } else if (otype == ')') {
        Y_ASSERT(!zs.empty() && (zs.back().SentBeg < br || (zs.back().SentBeg == br && zs.back().WordBeg <= wd)));
        zs.back().SentEnd = br;
        zs.back().WordEnd = wd;
    }
}

void TDirectZones::StoreZone(ui8 ztype, const TStringBuf& key, TPosting beg, TPosting end) {
    TVector<TZoneSpan>& zs = InsertOrGetZone(TCiString(key), ztype);

    if (zs.empty()) {
        zs.push_back(TZoneSpan(beg, end));
    } else {
        Y_ASSERT(beg <= end);
        Y_ASSERT(zs.back() < TZoneSpan(beg, end)); // zones must be sorted and have no duplicates

        if (zs.back().SentEnd < GetBreak(beg) || (zs.back().SentEnd == GetBreak(beg) && zs.back().WordEnd <= GetWord(beg))) {
            zs.push_back(TZoneSpan(beg, end));
        } else if (zs.back().SentEnd < GetBreak(end) || (zs.back().SentEnd == GetBreak(end) && zs.back().WordEnd < GetWord(end))) {
            zs.back().SentEnd = GetBreak(end);
            zs.back().WordEnd = GetWord(end);
        }
    }
}

void TDirectZones::StoreArchiveZoneAttr(ui8 type, const TStringBuf& name, const TWtringBuf& value, TPosting pos, bool noFollow, bool sponsored, bool ugc) {
    TVector<TDirectAttrEntry>& aes = AttrMap[TDZAttrKey(TCiString(name), type)];
    aes.push_back(TDirectAttrEntry());

    TDirectAttrEntry& ae = aes.back();
    ae.Sent = GetBreak(pos);
    ae.Word = GetWord(pos);
    ae.NoFollow = noFollow;
    ae.Sponsored = sponsored;
    ae.Ugc = ugc;
    ae.AttrValue = value.data();
}

void TDirectZones::Prepare() {
    Y_ASSERT(Zones.empty() && Attrs.empty());

    Zones.reserve(ZoneMap.size());

    for (TZoneMap::const_iterator i = ZoneMap.begin(); i != ZoneMap.end(); ++i) {
        TDirectTextZone z;
        z.Zone = i->first.data();
        z.ZoneType = i->second.first;
        z.Spans = i->second.second.data();
        z.SpanCount = (ui32)i->second.second.size();
        Zones.push_back(z);
    }

    Attrs.reserve(AttrMap.size());

    for (TAttrMap::const_iterator i = AttrMap.begin(); i != AttrMap.end(); ++i) {
        TDirectTextZoneAttr a;
        a.AttrName = i->first.Name.data();
        a.AttrType = i->first.Type;
        a.Entries = i->second.data();
        a.EntryCount = (ui32)i->second.size();
        Attrs.push_back(a);
    }
}

TVector<TZoneSpan>& TDirectZones::InsertOrGetZone(const TCiString& key, const ui8 ztype) {
    TZoneMap::iterator zi = ZoneMap.find(key);

    if (zi == ZoneMap.end()) {
        zi = ZoneMap.insert(std::make_pair(key, std::make_pair(ztype, TVector<TZoneSpan>()))).first;
    }

    return zi->second.second;
}


TDirectText2::TDirectText2(size_t wordCount, bool kiwiTrigger)
    : KiwiTrigger(kiwiTrigger)
    , Pool(100000 * wordCount/*, "TDirectText::Pool"*/)
    , SpacePool(100000 * wordCount/*, "TDirectText::SpacePool"*/)
{
    if (!KiwiTrigger)
        Pool.alloc_first_seg();
    SpacePool.alloc_first_seg();
    Entries.reserve(wordCount);
    TempSpaces.reserve(3);
}

void TDirectText2::Clear() {
    if (!KiwiTrigger)
        Pool.restart();
    SpacePool.restart();
    Entries.clear();
    Y_ASSERT(Entries.capacity() > 0);
    TempSpaces.clear();
    DirectZones.Clear();
    DirectAttrs.Clear();
}

TDirectTextEntries TDirectText2::GetEntries() {
    if (Entries.empty())
        return TDirectTextEntries(nullptr, 0);
    return TDirectTextEntries(&Entries[0], Entries.size());
}

size_t TDirectText2::MemUsage() const {
    return Pool.capacity() + Entries.capacity() + SpacePool.capacity() + DirectZones.Capacity();
}

void TDirectText2::InsertSpace(const wchar16* space, ui32 length, TBreakType type) {
    if (space && *space) {
        if (Entries.empty()) { // документ начался с пробелов
            Y_ASSERT(SpacePool.size() == 0);
            InsertForm(nullptr, nullptr, 0, 0, 0);
        } else {
            if (!IsSentBrk(type) && Entries.back().Token.IsInited() &&
                !TempSpaces.empty() && IsSentBrk(TempSpaces.back().Type))
            { // новое предложение началось с пробелов, не приклеивать их к предыдущему
                InsertForm(nullptr, nullptr, 0, 0, 0);
            }
        }
        if (!KiwiTrigger)
            space = Pool.append(space, length);
        if (!TempSpaces.empty() && TempSpaces.back().Length == 0 && IsZoneCls(TempSpaces.back().Type)) {
            TDirectTextSpace& sp = TempSpaces.back();
            sp.Space = space;
            sp.Length = length;
            sp.Type |= type;
        } else {
            TempSpaces.push_back(TDirectTextSpace());
            TDirectTextSpace& sp = TempSpaces.back();
            sp.Space = space;
            sp.Length = length;
            sp.Type = type;
        }
    } else {
        if (Entries.empty()) // разметочные брейки до первого токена
            return;
        if (IsParaBrk(type)) {
            if (!Entries.back().Token.IsInited()) // только пробелы после предыдущего брейка
                return;
            if (TempSpaces.empty()) {
                TempSpaces.push_back(TDirectTextSpace());
                TempSpaces.back().Space = space;
                TempSpaces.back().Length = length;
            }
            TempSpaces.back().Type |= ST_PARABRK;
        }
        if (IsZoneCls(type) && !TempSpaces.empty()) {
            if (Entries.back().Token.IsInited() && IsSentBrk(TempSpaces.back().Type))
            { // новое предложение началось с пробелов, не приклеивать их к предыдущему
                InsertForm(nullptr, nullptr, 0, 0, 0);
            }
            TempSpaces.push_back(TDirectTextSpace());
            TempSpaces.back().Space = space;
            TempSpaces.back().Length = length;
            TempSpaces.back().Type = ST_ZONECLS;
        }
    }
}

void TDirectText2::InsertForm(const TWtringBuf& token, TLemmatizedToken* forms, ui32 count, TPosting pos, ui32 origoffset) {
    AdjustLastEntry(pos);
    Entries.push_back(TDirectTextEntry2());
    TDirectTextEntry2& ent = Entries.back();
    ent.Posting = pos;
    ent.OrigOffset = origoffset;
    ent.Token = token.data();
    ent.LemmatizedToken = forms;
    ent.LemmatizedTokenCount = count;
    ent.Spaces = nullptr;
    ent.SpaceCount = 0;
}

void TDirectText2::StoreZone(ui8 type, const TStringBuf& key, TPosting pos) {
    if (key[0] == ')') {
        InsertSpace(nullptr, 0, ST_ZONECLS);
    }
    DirectZones.StoreZone(type, key, pos);
}

void TDirectText2::StoreZoneAttr(ui8 type, const TStringBuf& name, const TWtringBuf& value, TPosting pos, bool noFollow, bool sponsored, bool ugc) {
    if (KiwiTrigger) {
        DirectZones.StoreArchiveZoneAttr(type, name, value, pos, noFollow, sponsored, ugc);
        return;
    }
    wchar16* v = Pool.append(value.data(), value.size() + 1);
    v[value.size()] = 0;
    DirectZones.StoreArchiveZoneAttr(type, name, TWtringBuf(v, value.size()), pos, noFollow, sponsored, ugc);
}

void TDirectText2::OnCommitDoc() {
    AdjustLastEntry(0);
    DirectAttrs.Prepare();
    DirectZones.Prepare();
}

void TDirectText2::FillDirectData(TDirectData* data, const TDirectTextData2* extra) const {
    data->DirectText.Entries = Entries.data();
    data->DirectText.EntryCount = Entries.size();

    const TDirectZones::TZones& zones = DirectZones.GetZones();
    const TDirectZones::TAttrs& zoneAttrs = DirectZones.GetAttrs();

    if (extra) {
        data->Zones.assign(zones.begin(), zones.end());
        data->ZoneAttrs.assign(zoneAttrs.begin(), zoneAttrs.end());

        data->Zones.insert(data->Zones.end(), extra->Zones, extra->Zones + extra->ZoneCount);
        data->ZoneAttrs.insert(data->ZoneAttrs.end(), extra->ZoneAttrs, extra->ZoneAttrs + extra->ZoneAttrCount);

        data->DirectText.Zones = data->Zones.data();
        data->DirectText.ZoneCount = data->Zones.size();

        data->DirectText.ZoneAttrs = data->ZoneAttrs.data();
        data->DirectText.ZoneAttrCount = data->ZoneAttrs.size();
    } else {
        data->DirectText.Zones = zones.data();
        data->DirectText.ZoneCount = zones.size();

        data->DirectText.ZoneAttrs = zoneAttrs.data();
        data->DirectText.ZoneAttrCount = zoneAttrs.size();
    }

    const TDirectAttrs::TAttrs& sentAttrs = DirectAttrs.GetAttrs();
    data->DirectText.SentAttrs = sentAttrs.data();
    data->DirectText.SentAttrCount = sentAttrs.size();
}

void TDirectText2::AdjustLastEntry(TPosting pos) {
    if (!Entries.empty()) {
        TDirectTextEntry2& ent = Entries.back();
        ent.SpaceCount = (ui32)TempSpaces.size();
        if (ent.SpaceCount) {
            ent.Spaces = SpacePool.append(&TempSpaces[0], ent.SpaceCount);
        }
        if (ent.Token == nullptr)
            ent.Posting = pos;
    }
    TempSpaces.clear();
}

} // NIndexerCorePrivate
} // NIndexerCore

