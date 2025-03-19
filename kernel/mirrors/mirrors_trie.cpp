#include "mirrors_trie.h"
#include "mirrors_status.h"

#include <util/generic/ylimits.h>
#include <util/system/mlock.h>

const TMirrorsMappedTrie::THostId TMirrorsMappedTrie::MasterFlag =
    TMirrorsMappedTrie::THostId(1) << (std::numeric_limits<TMirrorsMappedTrie::THostId>::digits + std::numeric_limits<TMirrorsMappedTrie::THostId>::is_signed - 1);
const TMirrorsMappedTrie::THostId TMirrorsMappedTrie::LinkFlag = TMirrorsMappedTrie::MasterFlag >> 1;
const TMirrorsMappedTrie::THostId TMirrorsMappedTrie::GroupEndFlag = TMirrorsMappedTrie::LinkFlag;
const TMirrorsMappedTrie::THostId TMirrorsMappedTrie::HostIdMask = TMirrorsMappedTrie::LinkFlag - 1;

TMirrorsMappedTrie::TMirrorsMappedTrie(const char *fname, int prechargeMode)
    : TPrechargableFile(fname, prechargeMode)
    , Groups(nullptr)
    , GroupsExtended(nullptr)
    , Flags(nullptr)
    , FlagsSize(0)
{
    if (fname) {
        Map(fname, prechargeMode);
    }
}

void TMirrorsMappedTrie::Map(const char *fname, int prechargeMode) {
    Load(fname, TPrechargableFile::PrechargeModeToLoadMode(prechargeMode));
}

void TMirrorsMappedTrie::Load(const char *fname, EDataLoadMode loadMode) {
    TPrechargableFile::LoadFile(fname, loadMode);

    ui8 *const begin = (ui8*)TPrechargableFile::Start;
    ui8* pos = Trie.FromMemoryMap(begin);

    if (!pos || pos - begin > (ssize_t)TPrechargableFile::Length) {
        ythrow yexception() << "bad trie size for file: " << fname;
    }

    GroupsSize = acr<ui64>(pos);
    const ui64 groupsExtendedSize = acr<ui64>(pos);
    const ui64 groupsSizeOf = LCPD_SAVE_ALIGN(GroupsSize * sizeof(THostId));
    const ui64 groupsExtendedSizeOf = groupsExtendedSize * sizeof(THostId);
    Groups = (THostId*)pos;
    GroupsExtended = (THostId*)(pos + groupsSizeOf);

    if (pos - begin + groupsSizeOf + groupsExtendedSizeOf < TPrechargableFile::Length) {
        ui8* flagspos=pos+groupsSizeOf+groupsExtendedSizeOf;
        FlagsSize = acr<ui64>(flagspos);
        Flags=(ui32*)(flagspos);
    }

    if (pos - begin + groupsSizeOf > TPrechargableFile::Length) {
        ythrow yexception() << "bad groups size for file: " << fname;
    }

    if (pos - begin + groupsSizeOf + groupsExtendedSizeOf > TPrechargableFile::Length) {
        ythrow yexception() << "bad groups extent size for file: " << fname;
    }
}

TMirrorsMappedTrie::THostId TMirrorsMappedTrie::GetMain(THostId hostId) const {
    Y_ENSURE(IsValidHostId(hostId), "Invalid hostId");
    const THostId info = *(Groups + hostId);
    return IsMaster(info) ? hostId : (!IsLink(info) ? GetValue(info) : *(GroupsExtended + GetValue(info)));
}

int TMirrorsMappedTrie::IsMain(THostId hostId) const {
    Y_ENSURE(IsValidHostId(hostId), "Invalid hostId");
    return IsMaster(*(Groups + hostId));
}

int TMirrorsMappedTrie::IsSoft(THostId hostId) const {
    Y_ENSURE(IsValidHostId(hostId), "Invalid hostId");
    return GetFlags(hostId) & NLibMdb::BITFLAG_SOFT_MIRROR ? 1:0;
}
//If no flags assume mirror is safe (old behaviour)
int TMirrorsMappedTrie::IsSafe(THostId hostId) const {
    Y_ENSURE(IsValidHostId(hostId), "Invalid hostId");
    return HasFlags(hostId) && (GetFlags(hostId) & NLibMdb::BITFLAG_UNSAFE_MIRROR) ? 0:1;
}

ui32 TMirrorsMappedTrie::GetFlags(THostId hostId) const {
    Y_ENSURE(IsValidHostId(hostId), "Invalid hostId");
    return hostId > FlagsSize ? 0 : *(Flags + hostId);
}

bool TMirrorsMappedTrie::HasFlags(THostId hostId) const {
    Y_ENSURE(IsValidHostId(hostId), "Invalid hostId");
    return hostId < FlagsSize;
}

TString TMirrorsMappedTrie::AddLang(const TStringBuf &str, const char *lang) const {
    return TString(lang) + "@" + str;
}

TMirrorsMappedTrie::THostId TMirrorsMappedTrie::StringToId(const char* c, const char *lang) const {
    Y_ASSERT(TPrechargableFile::Start != nullptr);
    TPrechargableFile::REQUEST();
    TStringToIdTracker<TTrie> tracker(c);
    THostId res = Trie.TrackPath(tracker).Id;
    if (!res && !strchr(c, '@')) {
        TString s = AddLang(c, lang);
        TStringBufToIdTracker<TTrie> tracker(s);
        res = Trie.TrackPath(tracker).Id;
    }
    return res;
}


TMirrorsMappedTrie::THostId TMirrorsMappedTrie::StringToId(const TStringBuf& str, const char *lang) const {
    Y_ASSERT(TPrechargableFile::Start != nullptr);
    TPrechargableFile::REQUEST();
    TStringBufToIdTracker<TTrie> tracker(str);
    THostId res = Trie.TrackPath(tracker).Id;
    if (!res && str.find('@') != TString::npos) {
        TString s = AddLang(str, lang);
        TStringBufToIdTracker<TTrie> tracker(s);
        res = Trie.TrackPath(tracker).Id;
    }
    return res;
}

char* TMirrorsMappedTrie::IdToString(THostId hostId, char *buf, char *res) const {
    Y_ENSURE(IsValidHostId(hostId), "Invalid hostId");
    Y_ASSERT(TPrechargableFile::Start != nullptr);
    TIdToPCharTracker<TTrie> tracker(hostId, buf);
    Trie.TrackPath(tracker);
    if (res) strcpy(res, NRegion::UnknownRegion());
    if (tracker.Id) return nullptr;
    if (res) {
        char *p = strchr(buf, '@');
        if (!p) return buf;
        *p = 0;
        strcpy(res, NRegion::CanonizeRegion(buf));
        *p = '@';
        if (!strcmp(res, NRegion::UnknownRegion()))
            return buf;
        strcpy(buf, p + 1);
    }
    return buf;
}

TString& TMirrorsMappedTrie::IdToString(THostId hostId, TString &buf, char *res) const {
    Y_ENSURE(IsValidHostId(hostId), "Invalid hostId");
    Y_ASSERT(TPrechargableFile::Start != nullptr);
    buf.clear();
    buf.reserve(Trie.GetWordMaxLength() + 1);
    TIdToStrokaTracker<TTrie> tracker(hostId, buf);
    Trie.TrackPath(tracker);
    if (res) strcpy(res, NRegion::UnknownRegion());
    if (tracker.Id)
        buf.clear();
    else if (res) {
        size_t p = buf.find('@');
        if (p == TString::npos) return buf;
        strcpy(res, NRegion::CanonizeRegion(buf.substr(0,p).data()));
        if (!strcmp(res, NRegion::UnknownRegion()))
            return buf;
        buf.erase(0, p + 1);
    }
    return buf;
}

int TMirrorsMappedTrie::IsMain(const char* c, const char *lang) const {
    const THostId hostId = StringToId(c, lang);
    return hostId ? IsMain(hostId) : -1;
}

int TMirrorsMappedTrie::IsSoft(const char* c, const char *lang) const {
    const THostId hostId = StringToId(c, lang);
    return hostId ? IsSoft(hostId) : -1;
}
int TMirrorsMappedTrie::IsSafe(const char* c, const char *lang) const {
    const THostId hostId = StringToId(c, lang);
    return hostId ? IsSafe(hostId) : -1;
}

ui32 TMirrorsMappedTrie::GetFlags(const char* c, const char *lang) const {
    const THostId hostId = StringToId(c, lang);
    return hostId ? GetFlags(hostId) : 0;
}

bool TMirrorsMappedTrie::HasFlags(const char* c, const char *lang) const {
    const THostId hostId = StringToId(c, lang);
    return hostId ? HasFlags(hostId) : 0;
}

TMirrorsMappedTrie::THostId TMirrorsMappedTrie::GetCheck(const char *c, const char *lang) const {
    const THostId hostId = StringToId(c, lang);
    return hostId ? GetMain(hostId) : 0;
}

char* TMirrorsMappedTrie::GetCheck(const char *c, char *buf, const char *lang, char *res) const {
    if (const THostId hostId = StringToId(c, lang)) {
        const THostId mainId = GetMain(hostId);
        return IdToString(mainId, buf, res); //always convert - even if hostId == mainId : res can be ELR_MAX or ELR_*
    } else
        return nullptr;
}

TString& TMirrorsMappedTrie::GetCheck(const char *c, TString &buf, const char *lang, char *res) const {
    if (const THostId hostId = StringToId(c, lang)) {
        const THostId mainId = GetMain(hostId);
        buf = IdToString(mainId, buf, res);
        return buf;
    } else {
        buf.clear();
        return buf;
    }
}

TMirrorsMappedTrie::THostId TMirrorsMappedTrie::Check(const char *c, const char *lang) const {
    if (const THostId hostId = StringToId(c, lang)) {
        const THostId mainId = GetMain(hostId);
        return (hostId != mainId) ? mainId : 0;
    } else
        return 0;
}

char* TMirrorsMappedTrie::Check(const char *c, char *buf, const char *lang, char *res) const {
    const THostId mainId = Check(c, lang);
    return mainId ? IdToString(mainId, buf, res) : nullptr;
}

TString& TMirrorsMappedTrie::Check(const char *c, TString &buf, const char *lang, char *res) const {
    const THostId mainId = Check(c, lang);
    return mainId ? IdToString(mainId, buf, res) : (buf.clear(), buf);
}

char* TMirrorsMappedTrie::GetMain(const char *c, char *buf, const char *lang, char *res) const {
    char *const result = GetCheck(c, buf, lang, res);
    if (result) return result;
    strcpy(buf, c);
    if (res) strcpy(res, lang);
    return buf;
}

TString& TMirrorsMappedTrie::GetMain(const char *c, TString &buf, const char *lang, char *res) const {
    if (GetCheck(c, buf, lang, res).empty()) {
        buf = c;
        if (res) strcpy(res, lang);
    }
    return buf;
}

TMirrorsMappedTrie::TGroup TMirrorsMappedTrie::GetGroup(THostId hostId) const {
    Y_ENSURE(IsValidHostId(hostId), "Invalid hostId");
    return TGroup(Groups + hostId, *this);
}

TMirrorsMappedTrie::THostId* TMirrorsMappedTrie::GetPos(THostId hostId) const {
    THostId begin = Groups[hostId];
    if (IsLink(begin)) {
        return GroupsExtended + GetValue(begin);
    } else {
        return nullptr;
    }
}

TMirrorsMappedTrie::TGroup TMirrorsMappedTrie::GetGroup(const char *c, const char *lang) const {
    return TGroup(Groups + StringToId(c, lang), *this);
}

TMirrorsMappedTrie::TCharBuffer TMirrorsMappedTrie::MakeCharBuffer() const {
    return TCharBuffer(new char[GetHostNameMaxLength() + 1]);
}

size_t TMirrorsMappedTrie::GetHostNameMaxLength() const {
    return Trie.GetWordMaxLength();
}

TMirrorsMappedTrie::TGroup::TGroup(const THostId* begin, const TMirrorsMappedTrie& owner)
:   Begin(begin)
,   Owner(owner)
{
    if (IsLink(*Begin)) {
        Size = 1;
        const THostId* pos = Owner.GroupsExtended + GetValue(*Begin);
        while (!(*pos & GroupEndFlag)) {
            pos++;
            Size++;
        }
        Y_ASSERT(Size > 2);
    } else {
        Size = (Begin != Owner.Groups) ? 2 : 0; // NOTE if unknown host -> begin == groups
    }
}

TMirrorsMappedTrie::THostId* TMirrorsMappedTrie::TGroup::GetPos() {
    if (IsLink(*Begin)) {
        return Owner.GroupsExtended + GetValue(*Begin);
    } else {
        return nullptr;
    }
}


TMirrorsMappedTrie::TGroupsIterator TMirrorsMappedTrie::BeginGroup() const {
    return TGroupsIterator(Groups + 1, *this);
}
