#include "trie.h"

#include <cerrno>

#include <util/generic/ylimits.h>
#include <util/ysaveload.h>

TTrieBase::TTrieBase() {
    memset(this, 0, sizeof(TTrieBase));
}
TTrieBase::~TTrieBase() {
    if (IsAllocated)
        Allocation.Dealloc();
    free(FastFirst);
    Data = nullptr;
    FastFirst = nullptr;
}

void TTrieBase::MakeFastFirst() {
    FastFirst = (ui8**)malloc(std::numeric_limits<ui8>::max() * sizeof(ui8*));
    memset(FastFirst, 0, std::numeric_limits<ui8>::max() * sizeof(ui8*));
    ui8* pos = GetSearchOffs();
    size_t sz = acr<td_chr>(pos);
    if (sz)
        pos += size_td_offs_info;
    td_offs_t* num = (td_offs_t*)ALIGN_PTR_X(size_td_offs, pos + sz * size_td_chr);
    for (size_t n = 0; n < sz; n++) {
        FastFirst[((td_chr*)pos)[n]] = (ui8*)num + num[n] + 1;
    }
}

template <bool StopOnlyForInfoNode>
ui8* TTrieBase::FindNodeImpl(const td_chr* input, int inpsz) {
    //    if (Y_UNLIKELY(!inpsz))
    //        return NULL;

    ui8* node = Data + TrieOffset; // + size_td_chr;

    while (true) {
        ui8* pos = node;
        const td_chr l = acr<td_chr>(pos);
        if (!inpsz--)
            return (StopOnlyForInfoNode && (l & td_info_flag)) ? nullptr : pos; // is it end of word?
        Y_UNUSED(l);                                                            //when StopOnlyForInfoNode is false, compiler see unused 'l'
        const td_chr sz = acr<td_chr>(pos);
        const ui8 offs_info = sz ? acr<ui8>(pos) : 0;
        const td_chr* start = (td_chr*)pos;
        pos += sz * sizeof(td_chr);
        //fprintf(stderr, "i: '%c', node = %li (%p .. %p)\n", *input, node - Data, start, pos);
        //for (ui8 *ii = (ui8*)start; ii != pos; ++ii) fprintf(stderr, "%i (%c) ", *ii, *ii);
        const td_chr* found = NTriePrivate::mybinsearch(start, (td_chr*)pos, *input++);
        //fprintf(stderr, "f: %p (%li)\n", found, found? found - Data : 0);
        if (!found)
            return nullptr;
        const td_offs_t offs = NTriePrivate::Decode<td_offs_t>(offs_info, pos, found - start);
        node = AbsOffset ? Data + offs : node + offs;
    }
}

ui8* TTrieBase::FindNode(const td_chr* input, int inpsz) {
    return FindNodeImpl<true>(input, inpsz);
}

const ui8* TTrieBase::FindPrefixNode(const td_chr* input, int inpsz) const {
    return const_cast<TTrieBase*>(this)->FindNodeImpl<false>(input, inpsz);
}

size_t TTrieBase::FindAllNodes(const td_chr* input, int inpsz, TNodesVec& res) const {
    res.clear();
    int inpszSav = inpsz;
    // BEGIN NOTE
    // this should always be a copy of FindNode() with minor changes
    ui8* node = Data + TrieOffset; // + size_td_chr;

    while (true) {
        ui8* pos = node;
        const td_chr l = acr<td_chr>(pos);
        if (!(l & td_info_flag))
            res.push_back(std::make_pair(inpszSav - inpsz, pos));
        if (!inpsz--)
            return res.size();
        const td_chr sz = acr<td_chr>(pos);
        const ui8 offs_info = sz ? acr<ui8>(pos) : 0;
        const td_chr* start = (td_chr*)pos;
        pos += sz * sizeof(td_chr);
        const td_chr* found = NTriePrivate::mybinsearch(start, (td_chr*)pos, *input++);
        if (!found)
            return res.size();
        const td_offs_t offs = NTriePrivate::Decode<td_offs_t>(offs_info, pos, found - start);
        node = AbsOffset ? Data + offs : node + offs;
    }

    // END NOTE
    return res.size();
}

size_t TTrieBase::GetSizeForSave() const {
    return LCPD_VERSION_BUF_SZ +
           sizeof(TrieOffset) +
           sizeof(AllocationOffset) +
           sizeof(WordMaxLength) +
           LCPD_SAVE_ALIGN(AllocationOffset);
}

int TTrieBase::Save(IOutputStream* file, ui32 eSig, ui32 dSig) const {
    lcpd_version_data current_version(eSig, dSig);
    ui8 version_buf[LCPD_VERSION_BUF_SZ];
    current_version.save(version_buf);

    file->Write(version_buf, LCPD_VERSION_BUF_SZ);
    SaveMany(file, TrieOffset, AllocationOffset, WordMaxLength);
    const size_t rwsz = LCPD_SAVE_ALIGN(AllocationOffset);
    file->Write(Data, rwsz);

    lprintf("trie saved: %lu\n", (long unsigned)rwsz);

    return 0;
}

int TTrieBase::Save(const char* fname, ui32 eSig, ui32 dSig) const {
    TOFStream file(fname);
    int res = Save(&file, eSig, dSig);
    file.Finish(); // We want an exception here, not in dtor.
    return res;
}

int TTrieBase::Load(const char* fname, ui32 eSig, ui32 dSig) {
    TIFStream file(fname);
    return Load(&file, eSig, dSig);
}

int TTrieBase::Load(IInputStream* file, ui32 eSig, ui32 dSig) {
    lprintf("Loading tree dict...\n");
    ui8 version_buf[LCPD_VERSION_BUF_SZ];
    file->Load(version_buf, LCPD_VERSION_BUF_SZ);

    lcpd_version_data file_version(0, 0);
    if (!file_version.parse(version_buf))
        return 1;

    lcpd_version_data current_version(eSig, dSig);
    if (!current_version.check_version(file_version))
        return 1;

    LoadMany(file, TrieOffset, AllocationOffset, WordMaxLength);
    size_t rwsz = LCPD_SAVE_ALIGN(AllocationOffset);
    Data = (ui8*)Allocation.Alloc(AllocationSize = rwsz);
    IsAllocated = true;
    file->Load(Data, rwsz);
    lprintf("Loaded tree dict...\n");
    return 0;
}

ui8* TTrieBase::FromMemoryMap(ui8* pos, ui32 eSig, ui32 dSig) {
    lcpd_version_data file_version(0, 0);
    if (!file_version.parse(pos))
        throw yexception() << "Could not parse trie header";
    lcpd_version_data current_version(eSig, dSig);
    if (!current_version.check_version(file_version))
        throw yexception() << "Incorrect trie version (see stderr for details)";

    pos += LCPD_VERSION_BUF_SZ;
    TrieOffset = acr<td_sz_t>(pos);
    AllocationOffset = acr<td_sz_t>(pos);
    WordMaxLength = acr<ui32>(pos);
    AllocationSize = LCPD_SAVE_ALIGN(AllocationOffset);
    Data = pos;
    pos += AllocationSize;
    IsAllocated = false;
    AbsOffset = false;

    return pos;
}

TTrieBase::TCharBuffer TTrieBase::MakeCharBuffer() const {
    return TCharBuffer(new char[GetWordMaxLength() + 1]);
}
