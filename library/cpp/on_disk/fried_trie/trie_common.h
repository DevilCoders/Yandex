#pragma once

#include <util/system/defaults.h>
#include <util/system/compat.h>
#include <util/system/yassert.h>
#include <util/generic/ymath.h>
#include <util/generic/vector.h>
#include <util/generic/algorithm.h>
#include <util/generic/ylimits.h>
#include <util/generic/bitops.h>
#include <utility>

#include <cstdlib>
#include <cstdio>

//==============================================================================
#if 1
#define lprintf(format, ...)                        \
    {                                               \
        if (NTriePrivate::Debug)                    \
            fprintf(stderr, format, ##__VA_ARGS__); \
    }
#define lflush() fflush(stderr)
#else
#define lprintf(format, ...) (void)0
#define lflush() (void)0
#endif
//==============================================================================

#ifndef ALIGN_OF
#if 1 || defined(_MSC_VER)
#define ALIGN_OF(t) __alignof(t)
#else
#define ALIGN_OF(t) __alignof__(t)
#endif
#endif

#if defined(_must_align2_) || defined(_must_align4_) || defined(_must_align8_)
#error class TTrie now not implemented with alignment
#endif

#ifdef _must_align4_
#define ALIGN_PTR_X(s, p) (ui8*)(((uintptr_t)(p) + ((s)-1)) & ~(uintptr_t)((s)-1))
#define ALIGN_OFFS_X(s, p) (((uintptr_t)(p) + ((s)-1)) & ~(uintptr_t)((s)-1))
#define ALIGN_ADD_PTR_X(s, p) (-(intptr_t)(p) & ((s)-1))
#else
#define ALIGN_PTR_X(s, p) (p)
#define ALIGN_OFFS_X(s, p) (p)
#define ALIGN_ADD_PTR_X(s, p) 0
#endif

#define ALIGN_PTR4(p) ALIGN_PTR_X(4, p)
#define ALIGN_OFFS4(p) ALIGN_OFFS_X(4, p)
#define ALIGN_ADD_PTR4(p) ALIGN_ADD_PTR_X(4, p)

typedef ui8 td_chr; //should be unsigned
using td_offs_t = i64;
using td_sz_t = ui64;

const size_t size_td_chr = sizeof(td_chr), size_td_offs = sizeof(td_offs_t),
             size_td_chr_offs = sizeof(td_chr) + sizeof(td_offs_t);
const td_chr td_ff = (td_chr)0xff,
             td_deleted_flag = 128,
             td_data_mask = td_deleted_flag - 1, // view l without accessories flags, like td_deleted_flag
    td_info_flag = td_deleted_flag >> 1,
             td_info_mask = td_info_flag - 1;
//const td_offs_t td_afss_info_flag = 1 << 31;
const td_offs_t td_data_sz_bits = 9;
const td_offs_t td_data_sz_shift = sizeof(td_offs_t) * 8 - td_data_sz_bits, td_data_sz_max = ((td_offs_t)1 << td_data_sz_bits) - 1, td_data_sz_mask = td_data_sz_max << td_data_sz_shift;
const td_offs_t td_afss_mask = ((td_offs_t)1 << td_data_sz_shift) - 1;
const size_t size_td_offs_info = 1 * size_td_chr;

#define LCPD_SAVE_ALIGN_M1 15
#define LCPD_VERSION_MAJOR 2
#define LCPD_VERSION_MINOR 1 // reserved
const int LCPD_VERSION_BUF_SZ = 64;

#define LCPD_SAVE_ALIGN(a) ((a + LCPD_SAVE_ALIGN_M1) & ~LCPD_SAVE_ALIGN_M1)

struct lcpd_version_data {
    lcpd_version_data(ui32 eSig, ui32 dSig);

    bool parse(const ui8* buf, bool quiet = false);
    void save(ui8* buf) const;
    bool check_version(const lcpd_version_data& other) const;

    bool is_little_endian;
    ui8 alignment_step;
    ui8 offset_type_size;
    ui8 character_type_size;
    ui32 version_major;
    ui32 version_minor;
    ui32 enum_type_sig;
    ui32 value_type_sig;
};

template <class T>
inline size_t fwr(const T& t, FILE* f) {
    return fwrite(&t, sizeof(T), 1, f);
}
template <class T>
inline size_t frd(T& t, FILE* f) {
    return fread(&t, sizeof(T), 1, f);
}
template <class T>
inline void acq(T& t, const char*& c) {
    t = *(T*)c;
    c += sizeof(T);
}
template <class T>
inline void acq(T& t, const ui8*& c) {
    t = *(T*)c;
    c += sizeof(T);
}

template <class T>
inline T acb(const ui8* const base, td_offs_t& offs) {
    T& t = *(T*)(base + offs);
    offs += sizeof(T);
    return t;
}

template <class T>
inline T acr(ui8*& c) {
    T& t = *(T*)c;
    c += sizeof(T);
    return t;
}

template <class T>
inline T acr(const ui8*& c) {
    const T& t = *(const T*)c;
    c += sizeof(T);
    return t;
}

template <class T>
inline void scr(const T& t, ui8*& c) {
    *(T*)c = t;
    c += sizeof(T);
}

namespace NTriePrivate {
    const int OffsetTypeShift = 6;
    typedef std::pair<const ui8*, ui8*> TEnds;
    const td_chr InternalFieldsSplitter = '\x01';

    enum ENodeOffsetType {
        NOT_FixedUnsigned = 0,
        NOT_FixedSigned = 1 << OffsetTypeShift,
        NOT_ShiftedUnsigned = 2 << OffsetTypeShift,
        NOT_ShiftedSigned = 3 << OffsetTypeShift,
    };

    inline ui8 GetType(ui8 info) {
        constexpr ui8 mask = ~static_cast<ui8>((1 << OffsetTypeShift) - 1);
        return info & mask; // >> OffsetTypeShift;
    }

    inline ui8 GetSize(ui8 info) {
        constexpr ui8 mask = (ui8(1) << OffsetTypeShift) - 1;
        return info ? (info & mask) + 1 : 0;
    }

    template <typename T>
    void GetRange(ui8 type, ui8 bits, const ui8* pos, ui8 size, T& min, T& max);

    template <typename T>
    TEnds OptimizeArray(ui8* const info, ui8* const pos, size_t size);

    template <typename T>
    T Decode(ui8 type, ui8 bits, const ui8* pos, size_t num);

    template <typename T>
    inline T Decode(ui8 info, const ui8* pos, size_t num) {
        return Decode<T>(GetType(info), GetSize(info), pos, num);
    }

    template <typename T>
    ui8* Encode(ui8 type, ui8 bits, ui8* pos, T value);

    inline const td_chr* mybinsearch(const td_chr* n, const td_chr* k, td_chr what) { //log search through sorted array
        while (n < k) {
            const td_chr* i = n + (uintptr_t)(k - n) / 2;
            if (what == *i)
                return i;
            if (what < *i)
                k = i;
            else
                n = i + 1;
        }
        return nullptr;
    }

    inline const td_chr* mybinsearch1(const td_chr* n, const td_chr* k, td_chr what) { //should check that
        while (n < k) {
            const td_chr* i = n + (uintptr_t)(k - n) / 2;
            if (what == *i)
                return i;
            if (what < *i)
                k = i;
            else
                n = i + 1;
        }
        return n;
    }

    struct TTreeIterItem {
        td_offs_t start_offs, end_offs;
        TTreeIterItem(td_offs_t, td_offs_t, td_chr, td_offs_t s, td_offs_t num)
            : start_offs(s)
            , end_offs(s + num * size_td_offs)
        {
        }
        td_offs_t GetLink(ui8* data) {
            td_offs_t child = *(td_offs_t*)(data + start_offs);
            start_offs += size_td_offs;
            return child;
        }
        bool HasData() const {
            return start_offs < end_offs;
        }
    };

    struct TTreeIterItemRT {
        /*const*/ td_offs_t node, start_offs, Num;
        td_offs_t Cur;
        td_offs_t start_chr;
        /*const*/ ui8 Type, Bits;
        TTreeIterItemRT(td_offs_t p, td_offs_t c, td_chr i, td_offs_t s, td_offs_t num)
            : node(p)
            , start_offs(s)
            , Num(num)
            , Cur(0)
            , start_chr(c - 1)
            , Type(GetType(i))
            , Bits(GetSize(i))
        {
        }

        td_offs_t GetLink(ui8* data) {
            td_offs_t child = node + Decode<td_offs_t>(Type, Bits, data + start_offs, (size_t)Cur++);
            start_chr++;
            return child;
        }
        bool HasData() const {
            return Cur < Num;
        }
    };

    template <typename T, class TItem>
    struct TTreeNodesIterator {
        typedef T TTrie;
        const TTrie& tree;
        TVector<TItem> v;
        td_offs_t p;
        size_t VSize;
        TTreeNodesIterator(const TTrie& t)
            : tree(t)
        {
            restart();
        }

        TTreeNodesIterator(const TTrie& t, const ui8* startNode)
            : tree(t)
        {
            restart(startNode);
        }

        inline bool pop() {
            v.pop_back();
            if (!--VSize) {
                p = 0;
                return false;
            }
            return true;
        }

        void restart() {
            restart_impl(tree.GetTrieOffset());
        }

        void restart(const ui8* startNode) { // for call after TTrie::FindNode()
            restart_impl(startNode - tree.GetData() - size_td_chr);
        }

        void restart_impl(td_offs_t offsetOfNodeBegin) {
            td_offs_t pos0 = p = offsetOfNodeBegin;
            pos0 += size_td_chr;
            td_offs_t num0 = acb<td_chr>(tree.GetData(), pos0);
            ui8 offs_info = tree.GetData()[pos0];
            v.clear();
            if (!num0) {
                p = 0;
                return;
            }
            pos0 += size_td_offs_info;
            td_offs_t c = pos0;
            pos0 = ALIGN_OFFS_X(size_td_offs, pos0 + num0 * size_td_chr);
            v.push_back(TItem(p, c, offs_info, pos0, num0));
            VSize = 1;
        }

        td_offs_t cur() const {
            return p;
        }
        td_offs_t operator*() const {
            return p;
        }
        void operator++() {
            while (true) {
                TItem& vBack = v.back();
                if (vBack.HasData()) {
                    td_offs_t pos2 = vBack.GetLink(tree.GetData());
                    //td_chr& l = *(td_chr*)(tree.GetData() + pos2);

                    p = pos2;
                    pos2 += size_td_chr;
                    td_offs_t num0 = acb<td_chr>(tree.GetData(), pos2);
                    if (!num0)
                        return;
                    ui8 offs_info = tree.GetData()[pos2];
                    pos2 += size_td_offs_info;
                    td_offs_t c = pos2;
                    pos2 = ALIGN_OFFS_X(size_td_offs, pos2 + num0 * size_td_chr);
                    v.push_back(TItem(p, c, offs_info, pos2, num0));
                    VSize++;
                    return;
                } else {
                    if (!pop())
                        return;
                }
            }
        }

        // for TTreeIterItemRT only
        template <class R>
        bool BuildString(R& res, typename TTrie::TLeafData*& leafData) const {
            td_chr& l = *(td_chr*)(tree.GetData() + p);
            if (l & td_info_flag) // not a leaf
                return false;
            Y_ASSERT(!v.empty());
            size_t sz = v.size() - (v.back().node == p);
            res.resize(sz);
            for (size_t n = 0; n < sz; n++)
                res[n] = (char)tree.GetData()[v[n].start_chr];
            leafData = TTrie::GetLeafData(tree.GetData() + p + size_td_chr);
            return true;
        }

    private:
        TTreeNodesIterator(const TTreeNodesIterator&);
        void operator=(const TTreeNodesIterator&);
    };

    template <typename T>
    struct tree_nodes_iterator2 {
        typedef T TTrie;
        const TTrie& tree;
        td_offs_t p;
        size_t NodeSize;
        tree_nodes_iterator2(const TTrie& t)
            : tree(t)
        {
            restart();
        }
        void restart() {
            Y_ASSERT(tree.GetTrieOffset() && tree.GetData());
            p = tree.GetTrieOffset();
            NodeSize = tree.GetTreenodeSize(tree.GetData() + p);
        }
        td_offs_t cur() const {
            return p;
        }
        td_offs_t operator*() const {
            return p;
        }
        void operator++() {
            if (p) {
                p += NodeSize;
                if (p >= (td_offs_t)tree.GetAllocationOffset()) {
                    p = 0;
                    NodeSize = 0;
                } else {
                    NodeSize = tree.GetTreenodeSize(tree.GetData() + p);
                }
            }
        }
        size_t GetNodeSize() const {
            return NodeSize;
        }

    private:
        tree_nodes_iterator2(const tree_nodes_iterator2&);
        void operator=(const tree_nodes_iterator2&);
    };

    template <typename TTrie>
    struct treenode_hash_a {
        size_t operator()(const ui8* s) const {
            size_t l = acr<td_chr>(s) & td_data_mask, num = acr<td_chr>(s), n;
            if (num)
                s += size_td_offs_info;
            assert((l & size_td_chr_offs) == 1); //not needed, not implemented
            size_t h = (l + num * 13);
            for (n = num; n; --n)
                h = 5 * h + acr<td_chr>(s);
            s = ALIGN_PTR_X(size_td_offs, s);
            td_offs_t* offs = (td_offs_t*)s;
            for (n = num; n; ++offs, --n)
                h = 5 * h + *offs;
            if (TTrie::LeafDataSize && !(l & td_info_flag)) {
                s = ALIGN_PTR_X(TTrie::LeafDataAlignment, (ui8*)offs);
                h = 5 * h + (size_t) * (typename TTrie::TLeafData*)s; //data!! **FIX**
            }
            return size_t(h);
        }
    };

    template <typename TTrie>
    struct treenode_cmp_a {
        bool operator()(const ui8* a, const ui8* b) const {
#if 0
        if (a == b)
            return true;
#endif
            size_t l, num;
            if ((l = (acr<td_chr>(a) & td_data_mask)) != (acr<td_chr>(b) & td_data_mask) || (num = acr<td_chr>(a)) != acr<td_chr>(b))
                return false;
            if (num) {
                a += size_td_offs_info;
                b += size_td_offs_info;
            }
            assert((l & size_td_chr_offs) == 1); //not needed, not implemented

            // td_chr[sz] compare
            {
                const size_t sz1 = num * size_td_chr;

                if (memcmp(a, b, sz1))
                    return false;

                a += sz1;
                b += sz1;
            }

            // td_offs_t[sz] compare
            {
                a = ALIGN_PTR_X(size_td_offs, a);
                b = ALIGN_PTR_X(size_td_offs, b);
                const size_t sz2 = num * size_td_offs;

                if (memcmp(a, b, sz2))
                    return false;

                a += sz2;
                b += sz2;
            }

            // data compare
            if (TTrie::LeafDataSize && !(l & td_info_flag)) {
                a = ALIGN_PTR_X(TTrie::LeafDataAlignment, a);
                b = ALIGN_PTR_X(TTrie::LeafDataAlignment, b);

                return *(typename TTrie::TLeafData*)a == *(typename TTrie::TLeafData*)b;
            } else {
                return true;
            }
            /*
        num = num * size_td_chr_offs + ALIGN_ADD_PTR_X(size_td_offs, (2 + num) * size_td_chr + (num ? size_td_offs_info : 0));
        if (TTrie::LeafDataSize && !(l & td_info_flag))
            num += ALIGN_ADD_PTR_X(TTrie::LeafDataAlignment, num) + TTrie::LeafDataSize;
        return !memcmp(a, b, num);
 */
        }
    };

    extern bool Debug;

}

template <typename T>
T NTriePrivate::Decode(ui8 type, ui8 bits, const ui8* pos, size_t num) {
    if (Y_UNLIKELY(!(type | bits))) {
        return ((T*)ALIGN_PTR_X(ALIGN_OF(T), pos))[num];
    }

    using namespace NTriePrivate;

    Y_ASSERT(bits % 8 == 0); // not implemented bits precision
    bits /= 8;
    Y_ASSERT(bits > 0);
    Y_ASSERT(type == NOT_FixedSigned || type == NOT_FixedUnsigned);

    pos += bits * num;
    T result = (type == NOT_FixedSigned && pos[0] & 128) ? ~T(0) : 0;

    for (int i = 0; i < bits; i++) {
        result <<= 8;
        result |= T(pos[i]);
    }

    Y_ASSERT(result != 0); // it's impossible condition for offsets and leaf counters
    return result;
}

template <typename T>
ui8* NTriePrivate::Encode(ui8 type, ui8 bits, ui8* pos, T value) {
    if (Y_UNLIKELY(!(type | bits))) {
        pos = ALIGN_PTR_X(ALIGN_OF(T), pos);
        *(T*)pos = value;
        return pos + sizeof(T);
    }

    using namespace NTriePrivate;

    Y_ASSERT(bits % 8 == 0); // not implemented bits precision
    bits /= 8;
    Y_ASSERT(bits > 0);
    Y_ASSERT(type == NOT_FixedSigned || type == NOT_FixedUnsigned);

    for (int i = 0; i < bits; i++)
        pos[i] = ui8(value >> 8 * (bits - i - 1));

    return pos + bits;
}

template <typename T>
void NTriePrivate::GetRange(ui8 type, ui8 bits, const ui8* pos, ui8 size, T& min, T& max) {
    Y_ASSERT(size != 0);
    min = max = Decode<T>(type, bits, pos, 0);

    for (size_t i = 1; i < size; i++) {
        const T v = Decode<T>(type, bits, pos, i);
        if (min > v)
            min = v;
        if (max < v)
            max = v;
    }
}

#if defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 3) || (__GNUC__ > 4))
//#   pragma GCC diagnostic push // don't worked for gcc 4.4
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

template <typename T>
NTriePrivate::TEnds NTriePrivate::OptimizeArray(ui8* const info, ui8* const pos, size_t size) {
    //    using namespace NTriePrivate;

    const ui8 oldType = GetType(*info), oldBits = GetSize(*info);

    T min, max;
    GetRange(oldType, oldBits, pos, (ui8)size, min, max);

    static_assert(sizeof(T) <= sizeof(ui64), "expect sizeof(T) <= sizeof(ui64)");
    const ui64 abs_max = Max(std::abs(i64(min)), std::abs(i64(max)));
    Y_ASSERT(min && max); // -> abs_max != 0

    ui8 newBits = (ui8)GetValueBitCount(abs_max);
#if defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 3) || (__GNUC__ > 4))
    const ui8 newType = (min >= 0) ? (ui8)NOT_FixedUnsigned : (ui8)NOT_FixedSigned;
    if (min < 0)
#else
    static const T signMask = T(1) << std::numeric_limits<T>::digits - 1 << 1;
    bool isSigned = std::numeric_limits<T>::is_signed && min & signMask; // Don't use const bool. It bring to gcc SIGSEGV (checked for gcc 4.4).
    const ui8 newType = (!isSigned) ? (ui8)NOT_FixedUnsigned : (ui8)NOT_FixedSigned;
    if (isSigned)
#endif
        newBits++;
    constexpr ui8 mask = ~static_cast<ui8>(7);
    newBits = (newBits + 7) & mask; // round to bytes
    Y_ASSERT(newBits <= 64 && newBits % 8 == 0);

    const size_t oldSize = (!*info) ? (ALIGN_ADD_PTR_X(ALIGN_OF(T), pos) + size * sizeof(T)) : (size * oldBits / 8);
    const size_t newSize = size * newBits / 8;

    if (*info && oldSize <= newSize) {
        return TEnds(pos + oldSize, pos + oldSize);
    }

    Y_ASSERT(!(oldSize < newSize));

    ui8* writePos = pos;
    for (size_t i = 0; i < size; i++)
        writePos = Encode(newType, newBits, writePos, Decode<T>(oldType, oldBits, pos, i));

    *info = (newBits - 1) | newType;
    Y_ASSERT(*info);
    //    printf("AAA info = %u\n", *info);
    return TEnds(pos + oldSize, pos + newSize);
}

#if defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 3) || (__GNUC__ > 4))
//#   pragma GCC diagnostic pop // don't worked for gcc 4.4
#endif
