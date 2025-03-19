#pragma once

#include <kernel/trie_common/prechargable_file.h>

#include <library/cpp/on_disk/fried_trie/trie.h>
#include <library/cpp/on_disk/fried_trie/trie_enumerated_trackers.h>
#include <library/cpp/region/regcodes.h>


class TMirrorsMappedTrie : public TPrechargableFile {
public:
    typedef ui32 THostId;
    typedef ::TTrie<TLeafCounter, NTriePrivate::TVoid>  TTrie;
    typedef TArrayPtr<char> TCharBuffer;

private:
    static const THostId MasterFlag, GroupEndFlag;
    static const THostId LinkFlag, HostIdMask;
    static Y_FORCE_INLINE bool IsMaster(const THostId hostId)    { return hostId & MasterFlag; }
    static Y_FORCE_INLINE bool IsLink(const THostId hostId)      { return hostId & LinkFlag; }

    TString AddLang(const TStringBuf &str, const char *lang) const;
public:
    // for trie builder:
    static Y_FORCE_INLINE THostId GetValue(const THostId hostId) {
        return hostId & HostIdMask;
    }
    static Y_FORCE_INLINE THostId SetMaster(THostId id) {
        return id | MasterFlag;
    }
    static Y_FORCE_INLINE THostId SetLink(THostId id) {
        return id | LinkFlag;
    }
    static Y_FORCE_INLINE void SetGroupEnd(THostId& id) {
        id |= GroupEndFlag;
    }

public:
    class TGroup {
        friend class TMirrorsMappedTrie;
        friend class TGroupsIterator;
    private:
        const THostId *const Begin;
        const TMirrorsMappedTrie& Owner;
        size_t Size;
        TGroup(const THostId* begin, const TMirrorsMappedTrie& owner);
    public:
        inline bool operator==(const TGroup& other) const;
        inline THostId operator[](size_t n) const;
        inline size_t GetSize() const { return Size; }
        THostId* GetPos();
    }; // class TGroup

    class TGroupsIterator {
        friend class TMirrorsMappedTrie;
    private:
        const THostId *Pos, *const End;
        const TMirrorsMappedTrie& Owner;
        inline TGroupsIterator(const THostId *pos, const TMirrorsMappedTrie& owner);
        inline void Next(const THostId* pos);
    public:
        TGroup operator*() const { return TGroup(Pos, Owner); }
        TGroupsIterator& operator++() { Next(Pos + 1); return *this; }
        bool operator==(const TGroupsIterator& other) const { return Pos == other.Pos; }
    }; // class TGroupsIterator
private:
    TTrie   Trie;

    THostId *Groups, *GroupsExtended;
    ui32 * Flags;
    ui64 FlagsSize;
    ui64 GroupsSize;

public:
    TMirrorsMappedTrie(const char *fname = nullptr, int prechargeMode = PCHM_Disable);
    ~TMirrorsMappedTrie() override { }

    // 0 - no, > 0 - yes, -1 - not found
    int IsMain(THostId hostId) const;
    int IsMain(const char* c, const char *lang = "RUS") const;

    THostId GetMain(THostId hostId) const;
    char*   GetMain(const char *c, char *buf, const char *lang = "RUS", char *resLang = nullptr) const;
    TString& GetMain(const char *c, TString &buf, const char *lang = "RUS", char *resLang = nullptr) const;

    /// Get main mirror name. Return null if c not found.
    THostId GetCheck(const char *c, const char * lang = "RUS") const;
    char*   GetCheck(const char *c, char *buf, const char *lang = "RUS", char *resLang = nullptr) const;
    TString& GetCheck(const char *c, TString &buf, const char *lang = "RUS", char *resLang = nullptr) const;

    /// Also returns 0 if c is main mirror
    THostId Check(const char *c, const char *lang = "RUS") const;
    char*   Check(const char *c, char *buf, const char *lang = "RUS", char *resLang = nullptr) const;
    TString& Check(const char *c, TString &buf, const char *lang = "RUS", char *resLang = nullptr) const;

    TGroup  GetGroup(THostId hostId) const;
    THostId* GetPos(THostId hostId) const;
    TGroup  GetGroup(const char *c, const char *lang = "RUS") const;

    ui32    GetFlags(THostId hostId) const;
    ui32    GetFlags(const char *c, const char *lang = "RUS") const;

    bool    HasFlags(THostId hostId) const;
    bool    HasFlags(const char *c, const char *lang = "RUS") const;

    int IsSoft(THostId hostId) const;
    int IsSoft(const char* c, const char *lang = "RUS") const;

    int IsSafe(THostId hostId) const;
    int IsSafe(const char* c, const char *lang = "RUS") const;

    THostId StringToId(const char* c, const char *lang = "RUS") const;
    THostId StringToId(const TStringBuf& str, const char *lang = "RUS") const;

    // if res !=NULL split a.ru#RUS:33 to a.ru:33 and RUS, b.ru to b.ru and UNK; if res == NULL do not split - return a.ru#RUS:33; if there is corrupted name a.ru#asdba then return UNK
    char*   IdToString(THostId hostId, char *buf, char *res = nullptr) const;
    TString& IdToString(THostId hostId, TString &buf, char *res = nullptr) const;

    TCharBuffer MakeCharBuffer() const;
    size_t GetHostNameMaxLength() const;
    inline bool IsValidHostId(THostId hostId) const { return hostId >= 1 && hostId <= GroupsSize; }

    TGroupsIterator BeginGroup() const;
    /*const*/ TGroupsIterator EndGroup() const { return TGroupsIterator(Groups + GroupsSize, *this); }

    void Map(const char *fname, int prechargeMode = PCHM_Disable);
    void Load(const char *fname, EDataLoadMode loadMode);
}; // class TMirrorsMappedTrie
//==============================================================================
// class TMirrorsMappedTrie::TGroup implemetation
bool TMirrorsMappedTrie::TGroup::operator==(const TGroup& other) const {
    if (IsLink(*Begin) != IsLink(*other.Begin))
        return false;
    return !IsLink(*Begin) ? (Begin == other.Begin || Owner.Groups + GetValue(*Begin) == other.Begin) : (GetValue(*Begin) == GetValue(*other.Begin));
}

TMirrorsMappedTrie::THostId TMirrorsMappedTrie::TGroup::operator[](size_t n) const {
    Y_ASSERT(n < GetSize());
    const THostId info = *Begin;
    return GetValue(!IsLink(info) ? ((((!n && IsMaster(info))) || (n && !IsMaster(info))) ? *(Owner.Groups + GetValue(info)) : info) : *(Owner.GroupsExtended + GetValue(info) + n));
}

TMirrorsMappedTrie::TGroupsIterator::TGroupsIterator(const THostId *pos, const TMirrorsMappedTrie& owner)
:   Pos(pos)
,   End(owner.Groups + owner.GroupsSize)
,   Owner(owner)
{
    Next(pos);
}

void TMirrorsMappedTrie::TGroupsIterator::Next(const THostId* pos) {
    Pos = pos;
    while (Pos != End && !IsMaster(*Pos))
        Pos++;
}
//==============================================================================
