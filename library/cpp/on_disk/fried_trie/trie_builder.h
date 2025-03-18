#pragma once

#include "trie.h"
//==============================================================================
#include <library/cpp/microbdb/safeopen.h>
#include <util/generic/hash_set.h>
#include <library/cpp/map_text_file/map_tsv_file.h>
#include <util/string/cast.h>
#include <util/generic/map.h>
#include <util/system/fs.h>
#include <util/system/fstat.h>
#include <util/string/printf.h>
#include <util/generic/ptr.h>
#include <inttypes.h>
#include <library/cpp/logger/global/global.h>
//==============================================================================

template <typename char_type, ui32 max_size, ui32 max_data_size>
struct TStrokaSortImpl: public TNonCopyable {
    typedef char_type TChar;
    static const ui32 MaxSize = max_size;
    static const ui32 MaxDataSize = max_data_size;
    static const ui32 RecordSig = 0x57537453; // StSW in little-endian
    ui32 Size;
    ui32 DataSize;

    TChar String[MaxSize];
    char DataBuf[MaxDataSize];

    TStrokaSortImpl() {
        String[0] = '\0';
    }
    template <class L, class S>
    TStrokaSortImpl(const TChar* s, ui32 size, const L& data, const S& serializer = S())
        : Size(size)
    {
        //if (Y_UNLIKELY(size >= MaxSize))
        //    ythrow yexception() << "String buffer is currently limited to " << MaxSize << " bytes, string is " << size;
        memcpy(String, s, Size);
        String[Size] = '\0';
        DataSize = serializer.ToMem((char*)(String + Size + 1), MaxDataSize, data);
    }

    inline bool operator<(const TStrokaSortImpl& b) const {
        int cmp = memcmp((const char*)String, (const char*)b.String, Min<ui32>(Size, b.Size));
        return cmp ? cmp < 0 : Size < b.Size;
    }

    inline size_t SizeOf() const {
        return ((const char*)String + Size + 1 + DataSize) - (const char*)this;
    }

    const char* Data() const {
        return (char*)(String + Size + 1);
    }
};

typedef TStrokaSortImpl<td_chr, 4096, td_data_sz_max - 2> TStrokaSort;

//==============================================================================

template <typename N, typename PL>
size_t TTrie<N, PL>::HowMuch(const td_offs_t offset, td_offs_t num) {
    size_t sz = num & td_afss_mask;
    size_t ldatasz1 = (num >> td_data_sz_shift) & td_data_sz_max;
    td_offs_t end = offset;
    end += (2 + sz) * size_td_chr; /*l + sz + td_chr[]*/
    end += sz ? size_td_offs_info : 0;
    end += ALIGN_ADD_PTR_X(size_td_offs, end)                                       /*alignment for SPARC, etc.*/
           + sz * size_td_offs;                                                     /*table*/
    end += ldatasz1 ? ALIGN_ADD_PTR_X(LeafDataAlignment, end) + (ldatasz1 - 1) : 0; /*if this can be ending*/
    ;
    end += TNodeData::HowMuch(end, sz);
    return end - offset;
}

template <typename N, typename PL>
size_t TTrie<N, PL>::GetTreenodeSize(const ui8* const s) {
    size_t l = ((td_chr*)s)[0], num = ((td_chr*)s)[1];
    const ui8* end = s;
    end += (1 + (l & td_info_mask)) * size_td_chr;
    ui8 offs_info = 0;
    if (num) {
        offs_info = *end;
        end += size_td_offs_info;
    }
    end += num * size_td_chr;
    end = (!offs_info) ? ALIGN_PTR_X(size_td_offs, end) + num * size_td_offs : end + num * NTriePrivate::GetSize(offs_info) / 8;
    if (LeafDataSize && !(l & td_info_flag)) {
        end = ALIGN_PTR_X(LeafDataAlignment, end);
        end += PL().MemSize((char*)end);
    }
    end = TNodeData::GetEnd(end, num);
    return end - s;
}

template <typename N, typename PL>
void TTrie<N, PL>::AllocNode(td_offs_t ptr_offs) {
    td_offs_t offs = AllocationOffset;
    td_offs_t num = *(td_offs_t*)(Data + ptr_offs);
    //lprintf("AllocNode: num = %ld\n", num);
    //lprintf("trie.AllocationOffset = %ld\n", (long)AllocationOffset);
    if (Allo(HowMuch(AllocationOffset, num)))
        abort();
    *(td_offs_t*)(Data + ptr_offs) = offs;
    //lprintf("AAA AllocNode(%ld) -> %ld\n", (long)ptr_offs, (long)offs);
    size_t ldatasz1 = (num >> td_data_sz_shift) & td_data_sz_max; // non-zero - is leaf, with possibly data of size ldatasz1-1
    num &= td_afss_mask;
    //lprintf("AllocNode: data[%li] = %ld\n", offs + 1, num);
    assert(num || ldatasz1 || offs == (td_offs_t)size_td_offs);
    ui8* pos = Data + offs;
    ((td_chr*)pos)[0] = td_chr(num ? (!ldatasz1 ? (td_info_flag | 1) : 1) : /*0*/ 1);
    ((td_chr*)pos)[1] = (td_chr)num;
    pos += 2 * size_td_chr;
    if (num && size_td_offs_info != 0) {
        *(td_chr*)pos = 0;
        pos += size_td_offs_info;
    }
    memset(pos, 255, num * size_td_chr);
    pos += num * size_td_chr;
    ui8* end = ALIGN_PTR_X(size_td_offs, pos) + num * size_td_offs;
    end = ALIGN_PTR_X(LeafDataAlignment, end) + (ldatasz1 ? ldatasz1 - 1 : 0);
    memset(pos, 0, end - pos);
    TNodeData::Init(end, num);
}

template <typename N, typename PL>
void TTrie<N, PL>::AllocForSymbSetup() {
    td_offs_t node = LastAllocationOffset, end = AllocationOffset;

    while (node < end) {
        td_offs_t pos = node;
        pos += size_td_chr;
        const ui8 num = acb<td_chr>(Data, pos);
        if (num)
            pos += size_td_offs_info;
        pos = ALIGN_OFFS_X(size_td_offs, pos + num);
        for (ui8 i = 0; i < num; i++, pos += size_td_offs)
            AllocNode(pos);
        node += GetTreenodeSize(Data + node);
    }

    LastAllocationOffset = end;
}

void TTrieBase::DataSetup(ui8* pos, const char* data, size_t dataSize) {
    size_t l = acr<td_chr>(pos), num = acr<td_chr>(pos);
    Y_UNUSED(l); // used in assertion only
    if (num)
        pos += size_td_offs_info;
    //printf("DataSetup: ok\n");
    assert(!(l & td_info_flag));
    pos += num * size_td_chr;
    pos = ALIGN_PTR_X(size_td_offs, pos) + num * size_td_offs;
    pos = ALIGN_PTR_X(LeafDataAlignment, pos);
    memcpy(pos, data, dataSize);
}

namespace NTriePrivate {
    using TParentAndNode = std::pair<td_offs_t*, ui8*>;
    using TTrieLayers = TVector<THolder<IOutputStream>>;

    ui32 inline MakeNodesLayer(ui8* const data, td_offs_t* const link, ui8* const node, TTrieLayers& layers) {
        ui8* pos = node;
        ui32 result = 0;
        const td_chr l = acr<td_chr>(pos), num = acr<td_chr>(pos);
        (void)l;

        if (num) {
            pos += size_td_offs_info + num * size_td_chr;
            pos = ALIGN_PTR_X(size_td_offs, pos);
            td_offs_t* offs = (td_offs_t*)pos;

            for (td_chr i = 0; i < num; i++, offs++)
                result = Max(result, MakeNodesLayer(data, offs, data + *offs, layers));

            result++;
        }

        TParentAndNode ln(link, node);
        layers[result]->Write(&ln, sizeof(ln));

        return result;
    }

    template <bool NeedTo>
    struct TNodePackInfo {
    };

    template <>
    struct TNodePackInfo<false> {
        const td_offs_t From;
        const td_offs_t SumDelta;

        inline TNodePackInfo(td_offs_t from, td_offs_t /*to*/, td_offs_t sumDelta = -1)
            : From(from)
            , SumDelta(sumDelta)
        {
        }

        static inline bool LessFrom(const TNodePackInfo& a, const TNodePackInfo& b) {
            return a.From < b.From;
        }

        static inline bool LessTo(const TNodePackInfo& /*a*/, const TNodePackInfo& /*b*/) {
            ythrow yexception() << "impossible call";
        }
    };

    template <>
    struct TNodePackInfo<true> : TNodePackInfo<false> {
        typedef TNodePackInfo<false> TSuper;
        const td_offs_t To;

        inline TNodePackInfo(td_offs_t from, td_offs_t to, td_offs_t sumDelta = -1)
            : TSuper(from, to, sumDelta)
            , To(to)
        {
        }

        static inline bool LessFrom(const TNodePackInfo& a, const TNodePackInfo& b) {
            return TSuper::LessFrom(a, b);
        }

        static inline bool LessTo(const TNodePackInfo& a, const TNodePackInfo& b) {
            return a.To < b.To;
        }
    };

    template <bool NeedTo>
    class TPackInfoBase: public TVector<TNodePackInfo<NeedTo>> {
    protected:
        typedef TVector<TNodePackInfo<NeedTo>> TStore;

        inline td_offs_t GetNodeShiftImpl(const TNodePackInfo<NeedTo>& pos, /// XXX must be !empty()
                                          bool (*cmpOp)(const TNodePackInfo<NeedTo>&, const TNodePackInfo<NeedTo>&)) const {
            Y_ASSERT(!this->empty());
            td_offs_t result = 0;
            typename TStore::const_iterator offIt = LowerBound(this->begin(), this->end(), pos, cmpOp);
            if (offIt == this->end()) {
                Y_ASSERT(offIt != this->begin());
                //            if (offIt != this->begin())
                result = (offIt - 1)->SumDelta;
            } else if (!cmpOp(pos, *offIt)) {
                result = offIt->SumDelta;
            } else {
                Y_ASSERT(offIt != this->begin());
                //            if (offIt != this->begin())
                result = (offIt - 1)->SumDelta;
            }
            return result;
        }

    public:
        inline td_offs_t GetNodeShiftFrom(td_offs_t pos) const { /// XXX must be !empty()
            Y_ASSERT(!this->empty());
            if (pos < this->front().From)
                return 0;
            return GetNodeShiftImpl(TNodePackInfo<NeedTo>(pos, 0), &TNodePackInfo<NeedTo>::LessFrom);
        }

        void reserve(size_t size) {
            lprintf("PackInfo.reserve(%zu * %zu)\n", sizeof(typename TStore::value_type), size);
            TStore::reserve(size);
        }
    };

    template <bool NeedTo>
    class TPackInfo {
    };

    template <>
    class TPackInfo<false>: public TPackInfoBase<false> {
    public:
        inline td_offs_t GetNodeShiftTo(td_offs_t /*pos*/) const {
            return 0;
        }
    };

    template <>
    class TPackInfo<true>: public TPackInfoBase<true> {
    public:
        inline td_offs_t GetNodeShiftTo(td_offs_t pos) const { /// XXX must be !empty()
            Y_ASSERT(!this->empty());
            if (pos < this->front().To)
                return 0;
            return this->GetNodeShiftImpl(TNodePackInfo<true>(0, pos), &TNodePackInfo<true>::LessTo);
        }
    };

}

template <typename N, typename PL>
void TTrie<N, PL>::MergeEndsImpl(const char* tmpdir) {
    using namespace NTriePrivate;
    typedef THashSet<const ui8*, treenode_hash_a<TSelf>, treenode_cmp_a<TSelf>> tn_cmp_hash_t;
    const TString fnameTemplate = TString::Join(tmpdir, "/trie_merge_layer.p%zu.tmp");

    const size_t childDepthMax = GetWordMaxLength() + 1;

    {
        TTrieLayers layers(childDepthMax);
        for (size_t i = 0; i < childDepthMax; i++)
            layers[i].Reset(new TOFStream(Sprintf(fnameTemplate.data(), i)));
        lprintf("MergeEnds: layers filling\n");
        MakeNodesLayer(Data, nullptr, Data + TrieOffset, layers);
        for (auto& p : layers) {
            p->Finish(); // We want an exception here, not in dtor.
        }
    }

    bool has_change = true;
    long saved_space = 0, kl_sz = 0;
    tn_cmp_hash_t h;

    size_t passes_no = 0;
    for (; passes_no < childDepthMax && has_change; passes_no++) {
        const TString fname = Sprintf(fnameTemplate.data(), passes_no);
        THolder<TIFStream> layer = MakeHolder<TIFStream>(fname);
        lprintf("MergeEnds: pass %zu\n", passes_no);
        has_change = false;
        TParentAndNode it;
        const size_t layerSize = GetFileLength(fname) / sizeof(it);
        h.clear();
        h.reserve(layerSize);
        for (size_t i = 0; i < layerSize; i++) {
            layer->Load(&it, sizeof(it));
            h.insert(it.second);
        }
        lprintf("progress = %zu, h.size() = %zu\n", layerSize, h.size());

        layer.Reset(new TIFStream(fname));

        for (size_t i = 0; i < layerSize; i++) {
            layer->Load(&it, sizeof(it));
            ui8* const node = it.second;
            typename tn_cmp_hash_t::iterator j = h.find(node);
            if (j != h.end() && *j != node) {
                //ui32 oldso = *(i.p_src);
                *it.first = *j - Data;
                *node |= td_deleted_flag;
                kl_sz++;
                saved_space += GetTreenodeSize(node);
                has_change = true;
            }
        }
        lprintf("kl_sz = %ld, saved_space = %ld\n", kl_sz, saved_space);
        layer.Destroy();
        NFs::Remove(fname);
    }
    for (; passes_no < childDepthMax; passes_no++)
        NFs::Remove(Sprintf(fnameTemplate.data(), passes_no));
    AllNodesNum = CalcAllNodesNum() - kl_sz;
    lflush();
}

template <typename N, typename PL>
void TTrie<N, PL>::ConvertToRelOffs() {
    Y_ASSERT(AbsOffset);
    AbsOffset = false;

    td_offs_t p = TrieOffset;
    int progress = 0;
    while (p < (td_offs_t)AllocationOffset) {
        size_t sz = GetTreenodeSize(Data + p);
        ui8* s = p + Data;
        Y_UNUSED(acr<td_chr>(s));
        size_t num = acr<td_chr>(s), n;
        if (num)
            s += size_td_offs_info;
        td_offs_t *offs = (td_offs_t*)ALIGN_PTR_X(size_td_offs, s + num * size_td_chr), base = p; //(ui8*)offs - Data;
        for (n = num; n; ++offs, --n) {
            *offs -= base;
        }
        p += sz;
        progress++;
    }
}

template <typename N, typename PL>
int TTrie<N, PL>::Defrag() {
    using namespace NTriePrivate;
    Y_ASSERT(AbsOffset);
    lprintf("Defrag: move\n");
    TPackInfo<false> fix;
    fix.reserve(AllNodesNum);
    td_offs_t p = TrieOffset, dst = p;
    int progress = 0;
    td_offs_t optimized = 0;
    while (p < (td_offs_t)AllocationOffset) {
        ui8* const node = Data + p;
        size_t sz = GetTreenodeSize(node);
        if (!(*(td_chr*)node & td_deleted_flag)) {
            if (optimized) {
                memmove(Data + dst, node, sz);
                fix.push_back(TNodePackInfo<false>(p, dst, optimized));
            }
            dst += sz;
        } else {
            optimized += sz;
        }
        p += sz;
        progress++;
    }
    if (fix.empty()) {
        lprintf("Defrag: not needed\n");
        return 0;
    }
    AllocationOffset = dst;
    lprintf("Defrag: update (dst = %ld, progress = %u)\n", (long)dst, progress);
    NTriePrivate::tree_nodes_iterator2<TSelf> i(*this);
    for (; *i; ++i) {
        ui8* s = *i + Data;
        Y_UNUSED(acr<td_chr>(s));
        size_t num = acr<td_chr>(s), n;
        if (num)
            s += size_td_offs_info;
        td_offs_t* offs = (td_offs_t*)ALIGN_PTR_X(size_td_offs, s + num * size_td_chr);
        for (n = num; n; ++offs, --n) {
            *offs -= fix.GetNodeShiftFrom(*offs);
        }
    }
    lflush();
    return 0;
}

template <typename N, typename PL>
ssize_t TTrie<N, PL>::OptimizeNodeOffsets(ui8* pos) {
    td_chr l = acr<td_chr>(pos), num = acr<td_chr>(pos);

    if (num) {
        ui8* offs_info = pos;
        pos += size_td_offs_info + num * size_td_chr;
        NTriePrivate::TEnds ends = NTriePrivate::OptimizeArray<td_offs_t>(offs_info, pos, num);
        const ui8* src = ends.first;
        ui8* dst = ends.second;

        if (src != dst) {
            if (LeafDataSize && !(l & td_info_flag)) {
                src = ALIGN_PTR_X(LeafDataAlignment, src);
                dst = ALIGN_PTR_X(LeafDataAlignment, dst);

                const TLeafData t = *(const TLeafData*)src;
                *(TLeafData*)dst = t;

                src += LeafDataSize;
                dst += LeafDataSize;
            }

            ends = TNodeData::Move(dst, src, num);
        }

        return ends.first - ends.second;
    }

    return 0;
}

template <typename N, typename PL>
ssize_t TTrie<N, PL>::OptimizeNodeData(ui8* pos) {
    Y_UNUSED(acr<td_chr>(pos));
    td_chr num = acr<td_chr>(pos);

    if (num) {
        Y_ASSERT(!*pos); // offsets must be in plain format
        pos += size_td_offs_info;
    }
    pos = ALIGN_PTR_X(size_td_offs, pos + num * size_td_chr_offs);
    const NTriePrivate::TEnds ends = TNodeData::Optimize(pos, num);
    return ends.first - ends.second;
}

template <typename N, typename PL>
size_t TTrie<N, PL>::Optimize(ssize_t (*optimize)(ui8*)) {
    if (AbsOffset)
        return OptimizeImpl<true>(optimize);
    else
        return OptimizeImpl<false>(optimize);
}

template <typename N, typename PL>
template <bool IsAbsOffset>
size_t TTrie<N, PL>::OptimizeImpl(ssize_t (*optimize)(ui8*)) {
    using namespace NTriePrivate;

    static const bool NeedTo = !IsAbsOffset;
    TPackInfo<NeedTo> packInfo;
    packInfo.reserve(LastOptimizedNodesNum);

    NTriePrivate::tree_nodes_iterator2<TSelf> it(*this);
    lprintf("Optimize: pack and move\n");
    size_t progress = 0;
    ssize_t optimized = 0;
    td_offs_t dst = *it;
    while (const td_offs_t curr = *it) {
        const size_t size = it.GetNodeSize();
        ++it; // go to next node before compress of current
        const ssize_t delta = optimize(Data + curr);
        Y_ASSERT(delta >= 0 && (ssize_t)size > delta);
        const size_t newSize = size - delta;
        if (optimized)
            memmove(Data + dst, Data + curr, newSize);
        dst += newSize;
        if (delta) {
            optimized += delta;
            packInfo.push_back(TNodePackInfo<NeedTo>(*it, dst, optimized));
        }
        progress++;
    }

    if (!packInfo.empty() && packInfo.back().From == 0)
        packInfo.pop_back(); // remove information about last node in mem-heap, if it was packed now too

    lprintf("Optimize: progress %lu / %lu\n", (unsigned long)packInfo.size(), (unsigned long)progress);

    if (packInfo.empty()) {
        lprintf("Optimize: AllocationOffset = %ld\n", (long)AllocationOffset);
        return 0;
    }

    AllocationOffset = dst;
    LastOptimizedNodesNum = packInfo.size();

    lprintf("Optimize: fix\n");
    for (it.restart(); *it; ++it) {
        td_offs_t start = *it;
        ui8* pos = Data + start;
        td_chr l = acr<td_chr>(pos), num = acr<td_chr>(pos);
        Y_UNUSED(l);
        if (!num)
            continue;
        const ui8 info = acr<ui8>(pos);
        const ui8 type = GetType(info), bits = GetSize(info);
        pos += num * size_td_chr;
        pos = ALIGN_PTR_X(size_td_offs, pos);
        ui8* writePos = pos;
        const td_offs_t selfShift = IsAbsOffset ? 0 : packInfo.GetNodeShiftTo(start);
        for (size_t i = 0; i < num; i++) {
            td_offs_t newOffset;
            const td_offs_t oldOffset = Decode<td_offs_t>(type, bits, pos, i);

            if (IsAbsOffset) {
                newOffset = oldOffset - packInfo.GetNodeShiftFrom(oldOffset);
            } else {
                newOffset = oldOffset + selfShift - packInfo.GetNodeShiftFrom(start + selfShift + oldOffset);
                Y_ASSERT(newOffset < 0 || newOffset >= (td_offs_t)it.GetNodeSize()); // parent and child must not overlaped
            }

            writePos = Encode(type, bits, writePos, newOffset);
        }
    }

    lprintf("Optimize: AllocationOffset = %ld\n", (long)AllocationOffset);
    lflush();
    return optimized;
}

template <typename N, typename PL>
size_t TTrie<N, PL>::OptimizeNodesOffsets() {
    return Optimize(&TSelf::OptimizeNodeOffsets);
}

template <typename N, typename PL>
size_t TTrie<N, PL>::OptimizeNodesData() {
    return NodeDataSize ? Optimize(&TSelf::OptimizeNodeData) : 0;
}

using TTrieInDatFileName = std::pair<TString, ui32>;
struct TTrieInDatFileNames: public TVector<TTrieInDatFileName> {
    bool IsTmp;
    TTrieInDatFileNames(bool isTmp = false)
        : IsTmp(isTmp)
    {
    }
    ~TTrieInDatFileNames() {
        if (IsTmp)
            for (iterator i = begin(); i != end(); ++i)
                NFs::Remove(i->first);
    }
};

inline bool operator<(const TTrieInDatFileName& a, const TTrieInDatFileName& b) {
    return a.second < b.second;
}

template <typename N, typename PL>
void TTrie<N, PL>::LoadFromDatFile(const char* fname, int firstCharsCount) {
    TTrieInDatFileNames fnames;
    fnames.push_back(TTrieInDatFileName(fname, 0));
    LoadFromDatFiles(fnames, firstCharsCount);
}

template <typename N, typename PL>
void TTrie<N, PL>::LoadFromDatFiles(const TTrieInDatFileNames& fnames, int firstCharsCount) {
    Y_ASSERT(!fnames.empty());
    AbsOffset = true;
    WordMaxLength = 0;
    td_offs_t a = AllocationOffset /*0*/;
    Allo(size_td_offs);
    *(td_offs_t*)(Data + a) = firstCharsCount;
    AllocNode(a);
    TrieOffset = a + size_td_offs;
    LastAllocationOffset = TrieOffset;
    const char* fname = nullptr;
    for (int n = 0;; n++) {
        lprintf("trie.LoadFromText:\t%i\n", n);
        fname = (UpperBound(fnames.begin(), fnames.end(), TTrieInDatFileName(nullptr, n)) - 1)->first.data();
        if (!SymbSetupLoop(fname, n))
            break;
        AllocForSymbSetup();
    }
    lprintf("trie.AllocationOffset = %ld\n", (long)AllocationOffset);
}

template <typename N, typename PL>
void TTrie<N, PL>::MergeEnds(const char* tmpdir) {
    MergeEndsImpl(tmpdir);
    Defrag();
}

template <typename N, typename PL>
size_t TTrie<N, PL>::Optimize(double optimizationThresold, bool reallocate) {
    LastOptimizedNodesNum = AllNodesNum;
    size_t result = OptimizeNodesData();
    while (const size_t rr = OptimizeNodesOffsets()) {
        result += rr;
        if (double(rr) / result <= optimizationThresold) {
            lprintf("Optimize early exit\n");
            break;
        }
    }
    if (reallocate)
        Data = (ui8*)Allocation.Realloc(LCPD_SAVE_ALIGN(AllocationOffset));
    return result;
}

template <typename N, typename PL>
typename N::TValue TTrie<N, PL>::Fry(const char* tmpdir, const double optimizationThresold) {
    MergeEnds(tmpdir);
    typename TNodeData::TValue res = TNodeData::PostProcessTrie(GetData(), GetData() + GetTrieOffset());
    ConvertToRelOffs();
    Optimize(optimizationThresold, true);
    return res;
}

const ui8* TTrieBase::TreeFindNext1ByteItem(const ui8* const data, const ui8* pos, const td_chr* input, int inpsz, int depth) {
    if (!depth)
        return pos;
    for (; depth > 0; depth--) {
        size_t sz = acr<td_chr>(pos);
        if (sz)
            pos += size_td_offs_info;
        const td_chr* start = (td_chr*)pos;
        pos += sz * size_td_chr;
        const td_chr* found = NTriePrivate::mybinsearch(start, (td_chr*)pos, *input++);
        if (!found) {
            printf("!found: inpsz = %i\n", inpsz);
            return nullptr;
        }
        pos = ALIGN_PTR_X(size_td_offs, pos);
        td_offs_t offs = ((td_offs_t*)pos)[found - start]; //need also a way to choose the number of bytes in offset (?)
        pos = data + offs;
        td_offs_t l = *(td_chr*)pos;
        pos += size_td_chr;
        if ((inpsz -= (l & td_info_mask)) <= 0) {
            if (inpsz == 0)
                return (l & td_info_flag && depth != 1) ? nullptr : pos; //fix that!
            return nullptr;
        }
        if (!l) {
            printf("l = 0, inpsz = %i\n", inpsz);
            abort();
            return inpsz ? nullptr : pos;
        }
        l = (l & td_info_mask) - 1;
        if (memcmp(pos, input, l * size_td_chr)) {
            printf("memcmp\n");
            return nullptr;
        }
        input += l;
        pos += l * size_td_chr;
    }
    return pos;
}

int TTrieBase::Reserve(size_t sz) { // returns zero if ok
    if (AllocationSize < sz) {
        if (!AllocationSize) {
            Data = (ui8*)Allocation.Alloc(AllocationSize = sz);
            IsAllocated = true;
        } else {
            const size_t oldAllocationSize = AllocationSize;
            while (AllocationSize < sz)
                AllocationSize = AllocationSize * 5 / 4;
            AllocationSize = LCPD_SAVE_ALIGN(AllocationSize);
            lprintf("realloc %zu -> %" PRIu64 "\n", oldAllocationSize, AllocationSize);
            Data = (ui8*)Allocation.Realloc(AllocationSize);
        }
        return Data == nullptr;
    }
    return 0;
}

int TTrieBase::Allo(size_t sz) { // returns zero if ok
    assert(IsAllocated || !Data);
    AllocationOffset += sz;
    return Reserve(AllocationOffset);
}

void TTrieBase::SymbSetupLoopIter(const td_chr* const inp, ssize_t vsz, const char* const ldata, size_t ldatasz, int n, ui8& prevc, ui8& prevc1, ui8*& prevpos, int& ssc) {
    if (vsz <= n) {
        prevc = prevc1 = 255;
        prevpos = nullptr;
        if (vsz != n || !ldatasz)
            return;
    }

    ui8* pos = (ui8*)TreeFindNext1ByteItem(Data, GetSearchOffs(), inp, n, n);

    assert(pos);
    pos--;
    if (vsz == n) {
        DataSetup(pos, ldata, ldatasz);
        return;
    }
    td_chr curc1 = vsz == n + 1 ? td_ff : inp[n + 1];
    if (prevpos == pos /*8-)*/ && prevc == inp[n] && prevc1 == curc1)
        return;
    prevpos = pos, prevc = inp[n], prevc1 = curc1;
    SymbSetup(pos, inp[n], vsz == n + 1 ? ldatasz + 1 : 0);
    ssc++;
}

int TTrieBase::SymbSetupLoop(const char* fname /*FILE* fin*/, int n) {
    int ssc = 0;
    ui8 prevc = 255, prevc1 = 255;
    ui8* prevpos = nullptr;
    typedef TInDatFile<TStrokaSort> TStrokaInDatFile;
    TStrokaInDatFile in("TStrokaInDatFile", 2u << 20, 0);
    in.Open(fname);

    while (const TStrokaSort* pt = in.Next()) {
        SymbSetupLoopIter(pt->String, pt->Size, pt->Data(), pt->DataSize, n, prevc, prevc1, prevpos, ssc);
        if (!n && WordMaxLength < pt->Size)
            WordMaxLength = pt->Size;
    }
    return ssc;
}

int TTrieBase::SymbSetup(ui8* pos, td_chr c, size_t dsz1) {
    pos += size_td_chr;
    size_t num = acr<td_chr>(pos);
    if (num)
        pos += size_td_offs_info;
    td_chr* chars = (td_chr*)pos;
    td_offs_t* offs = (td_offs_t*)ALIGN_PTR_X(size_td_offs, pos + num * size_td_chr);
    //ugly, isn't it?
    size_t n;
    assert(num <= 255); //??
    for (n = 0; n < num; n++) {
        //lprintf("chars[%zu] = %hhu\n", n, chars[n]);
        if (chars[n] == c)
            break;
        if (chars[n] == 255) {
            n = NTriePrivate::mybinsearch1(chars, chars + n, c) - chars; //returns symbol 'after' current

            //lprintf("chars[%zu]: %p, %hhu, num = %zu, c = %hhu\n", n, chars + n, chars[n], num, c);
            if (num > 0)
                for (ui32 k = num - 1; k > n; k--)
                    chars[k] = chars[k - 1], offs[k] = offs[k - 1];
            chars[n] = c, offs[n] = 0;
            break;
        }
    }
    //lprintf("chars[%zu] = %hhu\n", n, chars[n]);
    //end ugly (this can be slow, I don't really care)
    assert(n != num);
    assert(offs[n] != 255);
    if (dsz1) {
        Y_ASSERT(!(offs[n] & td_data_sz_mask));
        offs[n] |= dsz1 << td_data_sz_shift; //mark it whether it could be word end (data size + 1)
    } else
        offs[n]++;
    return 0;
}

template <class P, class L = TLess<typename P::first_type>>
struct Pair1stLess {
    L LessOp;
    Pair1stLess(const L& op = L())
        : LessOp(op)
    {
    }
    bool operator()(const P& a, const P& b) const {
        return LessOp(a.first, b.first);
    }
};

template <>
inline NTriePrivate::TVoid FromString<NTriePrivate::TVoid>(const char*, size_t) {
    return NTriePrivate::TVoid();
}

template <> // tmp. fix
inline ui8 FromString<ui8>(const char* data, size_t len) {
    return (ui8)FromString<ui32>(data, len);
}

template <class PL>
ui32 TTrieBase::MakeDatFiles(const char* finame, TTrieInDatFileNames& fonames,
                             const int textField, const int dataField, const char* tmpdir) {
    using namespace NTriePrivate;
    Y_ASSERT(!fonames.empty());
    typedef td_chr chr;
    static const chr chrMin = std::numeric_limits<chr>::min();
    TVector<bool> firstLetters(std::numeric_limits<chr>::max() - chrMin + 1, false);
    typedef TMap<size_t, size_t> TStringLength;
    TStringLength stringsLength;
    typedef TDatSorterMemo<TStrokaSort, TCompareByLess> TStrokaSorter;
    TStrokaSorter sorter("MakeTreeSorter", 256u << 20, 64u << 10, 2u << 20, 0);
    //    const TString sorterDir(finame, ".dir");
    //    Mkdir(~sorterDir, MODE0755);
    sorter.Open(tmpdir);
    size_t total = 0;
    typedef typename PL::ValueType ValueType;
    static const bool ValueTypeIsEmpty = TypeTraits<ValueType>::IsEmpty;
    const size_t maxField = Max(textField, ValueTypeIsEmpty ? 0 : dataField);
    lprintf("filling\n"); // \"%s/\"\n", ~sorterDir);
    size_t line_number = 0;
    for (const auto& line : TMapTsvFile(finame)) {
        if (line.empty() || (line.size() == 1 && line.front().empty()))
            continue; // skip empty lines
        if (maxField >= line.size())
            throw yexception() << "Too few fields (" << line.size() << "), must be " << maxField + 1 << " in " << finame << " line " << line_number << "\n";
        const char* c = nullptr;
        c = line[textField].data();
        size_t l = line[textField].length();
        const chr* const cc = (const chr*)c;
        stringsLength[l]++;
        total += l;
        ValueType data(ValueTypeIsEmpty ? ValueType() : FromString<ValueType>(line[dataField]));
        const TStrokaSort s(cc, l, data, PL());
        sorter.Push(&s);
        firstLetters[*cc - chrMin] = true;
        ++line_number;
    }
    size_t used = 0, minSize = std::numeric_limits<size_t>::max();
    ssize_t ioname = fonames.size() - 1;
    for (TStringLength::reverse_iterator it = stringsLength.rbegin(), end = stringsLength.rend(); it != end; ++it) {
        const size_t use = it->first * it->second;
        used += use;
        while (ioname >= 0 && (used * 100 / total) >= fonames[ioname].second) {
            fonames[ioname].second = minSize;
            ioname--;
        }
        minSize = it->first;
    }
    fonames.front().second = 0;
    lprintf("sorting\n"); // \"%s/\"\n", ~sorterDir);
    sorter.Sort();
    lprintf("writing\n"); // \"%s\"\n", foname);
    typedef TOutDatFile<TStrokaSort> TStrokaOutDatFile;
    typedef std::pair<ui32, TSimpleSharedPtr<TStrokaOutDatFile>> TFout;
    typedef TVector<TFout> TFouts;

    TFouts fouts;
    for (size_t i = 0, max = fonames.size(); i < max; i++) {
        TSimpleSharedPtr<TStrokaOutDatFile> fout(new TStrokaOutDatFile("MakeTreeOutDatFile", 64u << 10, 2u << 20, 0));
        fout->Open(fonames[i].first);
        fouts.push_back(std::make_pair(fonames[i].second, fout));
    }
    while (const TStrokaSort* rec = sorter.Next()) {
        TFouts::iterator it = UpperBound(fouts.begin(), fouts.end(), TFout(rec->Size, nullptr), Pair1stLess<TFout>());
        for (TFouts::reverse_iterator rit(it), rend = fouts.rend(); rit != rend; ++rit)
            rit->second->Push(rec);
    }
    sorter.Close();
    //    rmdir(~sorterDir);
    return std::count(firstLetters.begin(), firstLetters.end(), true);
}

template <class PL>
ui32 TTrieBase::MakeDatFilesFromYT(ICustomDataReaderPtr reader, TTrieInDatFileNames& fonames) {
    using namespace NTriePrivate;
    Y_ASSERT(!fonames.empty());
    typedef td_chr chr;
    static const chr chrMin = std::numeric_limits<chr>::min();
    TVector<bool> firstLetters(std::numeric_limits<chr>::max() - chrMin + 1, false);
    typedef TMap<size_t, size_t> TStringLength;
    TStringLength stringsLength;
    size_t total = 0;
    typedef typename PL::ValueType ValueType;
    static const bool ValueTypeIsEmpty = TypeTraits<ValueType>::IsEmpty;
    INFO_LOG << "Fillin data from YT table" << Endl;
    size_t line_number = 0;

    TVector<TString> strings;
    reader->Start();
    while(reader->HasNext()) {
        TString str = reader->Next();
        const char* c = nullptr;
        c = str.c_str();
        size_t l = str.length();
        const chr* const cc = (const chr*)c;
        stringsLength[l]++;
        total += l;
        strings.push_back(str);
        firstLetters[*cc - chrMin] = true;
        ++line_number;
    }
    size_t used = 0, minSize = std::numeric_limits<size_t>::max();
    ssize_t ioname = fonames.size() - 1;
    for (TStringLength::reverse_iterator it = stringsLength.rbegin(), end = stringsLength.rend(); it != end; ++it) {
        const size_t use = it->first * it->second;
        used += use;
        while (ioname >= 0 && (used * 100 / total) >= fonames[ioname].second) {
            fonames[ioname].second = minSize;
            ioname--;
        }
        minSize = it->first;
    }
    fonames.front().second = 0;
    typedef TOutDatFile<TStrokaSort> TStrokaOutDatFile;
    typedef std::pair<ui32, TSimpleSharedPtr<TStrokaOutDatFile>> TFout;
    typedef TVector<TFout> TFouts;

    TFouts fouts;
    for (size_t i = 0, max = fonames.size(); i < max; i++) {
        TSimpleSharedPtr<TStrokaOutDatFile> fout(new TStrokaOutDatFile("MakeTreeOutDatFile", 64u << 10, 2u << 20, 0));
        fout->Open(fonames[i].first);
        fouts.push_back(std::make_pair(fonames[i].second, fout));
    }

    INFO_LOG << "Writing data to temporary files" << Endl;
    TString dataStr = "0";
    for (const auto& str: strings) {
        ValueType data(ValueTypeIsEmpty ? ValueType() : FromString<ValueType>(dataStr));
        const TStrokaSort rec((const chr *)str.c_str(), str.length(), data, PL());
        TFouts::iterator it = UpperBound(fouts.begin(), fouts.end(), TFout(rec.Size, nullptr), Pair1stLess<TFout>());
        for (TFouts::reverse_iterator rit(it), rend = fouts.rend(); rit != rend; ++rit)
            rit->second->Push(&rec);
    }
    return std::count(firstLetters.begin(), firstLetters.end(), true);
}

