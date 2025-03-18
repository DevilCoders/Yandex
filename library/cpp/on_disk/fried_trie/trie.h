#pragma once

#include <cstring>
#include <cstdio>

#include <util/system/defaults.h>
#include <util/system/yassert.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/stream/file.h>
#include <utility>

#include <library/cpp/remmap/remmap.h>

#include "trie_common.h"

/** Here we deal with objects of type TTrie<N, L>, where
    N is one of: TVoid (no special meaning), TLeafCounter (word <-> id conversion support) defined in trie_enumerated.h
    L is either TVoid or (any?) POD-type, which is stored with each leaf node

    To build optimized TTrie file you need several things:
     1. text file with input strings (tab-delimited);
     2. directory for temporary files;
     3. some estimation of the amount of RAM the process will take
    Then call LoadFromTextFile(), Fry() and Save().
    Beware! textNF parameter of LoadFromTextFile is 1-based
*/

#define FRIED_TRIE_STANDARD_TRIE_SORTER_TEMPLATE "standard_trie_sorter.p%D.tmp"

namespace NTriePrivate {
    template <typename T>
    struct TypeTraits {
        static inline size_t SizeOf() {
            return sizeof(T);
        }
        static inline size_t AlignOf() {
            return ALIGN_OF(T);
        }
        static const bool IsEmpty = false;
    };

    struct TVoid {
        typedef i8 TValue;
        inline operator size_t() {
            return 0;
        }
        static inline size_t HowMuch(size_t, size_t) {
            return 0;
        }
        static inline const ui8* GetEnd(const ui8* const pos, size_t) {
            return pos;
        }
        static inline void Init(ui8*, size_t) {
        }
        static inline TEnds Move(ui8* dst, const ui8* src, size_t) {
            return TEnds(src, dst);
        }
        static inline TEnds Optimize(ui8* pos, size_t) {
            return TEnds(pos, pos);
        }
        static inline TValue PostProcessTrie(ui8* const, ui8*) {
            return 0;
        }
        static const ui32 RecordSig = 0;

        // TVoid as packer for itself! (see TSimplePacker)
        typedef TVoid ValueType;
        size_t MemSize(const TVoid&) const {
            return 0;
        }
        size_t MemSize(const char*) const {
            return 0;
        }
        size_t ToMem(char*, size_t, const TVoid&) const {
            return 0;
        }
        size_t FromMem(TVoid&, const char*) const {
            return 0;
        }
    };

    template <>
    struct TypeTraits<TVoid> {
        static inline size_t SizeOf() {
            return 0;
        }
        static inline size_t AlignOf() {
            return 1;
        }
        static const bool IsEmpty = true;
    };

    template <class D>
    class TRecordSig {
        static const ui32 RecordSig = D::RecordSig;
    };
    template <>
    struct TRecordSig<ui8> { static const ui32 RecordSig = 8; };
    template <>
    struct TRecordSig<ui32> { static const ui32 RecordSig = 32; };

    template <class D>
    struct TSimplePacker {
        typedef D ValueType;
        static const ui32 RecordSig = TRecordSig<ValueType>::RecordSig;
        size_t MemSize(const D&) const {
            return sizeof(D);
        }
        size_t MemSize(const char*) const {
            return sizeof(D);
        }
        size_t ToMem(char* dst, size_t maxDst, const D& data) const {
            if (Y_UNLIKELY(sizeof(D) > maxDst))
                ythrow yexception() << "Data buffer is currently limited to " << maxDst << " bytes, data size " << sizeof(D);
            memcpy(dst, &data, sizeof(D));
            return sizeof(D);
        }
        size_t FromMem(D& data, const char* src) const {
            memcpy(&data, src, sizeof(D));
            return sizeof(D);
        }
    };

}

#if 0
#ifdef ALIGN_OF
#undef ALIGN_OF
#endif
#endif

/** Dictionary element structure:
    td_chr l - number of symbols in continuous sequence plus 1 (currently 1) + flag;
    td_chr[l] - continuous sequence (currently empty)
  ! td_chr sz - number of (next) symbols; entry point for the first symbol
    td_chr offs_info - information about offsets format
    td_chr[sz] - next symbols
  # ALIGN at sizeof(td_offs_t) if necessary #
    td_offs_t[sz] - offsets of next elements (according to symbol found)
    TLeafData LeafData - data only for node with information
    TNodeData NodeData - data for each node
 **/

struct TTrieInDatFileNames;


class TTrieBase {
public:
    /// Abstracts the data source for trie building
    class ICustomDataReader {
    public:
        /// Start reading. Introduced to control the precise moment of opening reading stream
        virtual void Start() = 0;

        /// Switch to next position in the input sequence. Returns current data prior to position switch
        virtual TString Next() = 0;

        virtual TString GetData() = 0; ///< Get current data string
        virtual ui32 GetAuxData() = 0; ///< Get current auxilliary data piece

        virtual bool HasNext() = 0; ///< Check that the input sequence has next record
        virtual bool EndGroup() = 0; ///< For grouped data check that this is the last record in group

        virtual ~ICustomDataReader() = default;
    };
    using ICustomDataReaderPtr = TAtomicSharedPtr<ICustomDataReader>;
protected:
    ui8* Data; // binary data that is the trie itself
    td_sz_t TrieOffset, AllocationSize, AllocationOffset, LastAllocationOffset;
    ui8** FastFirst; // direct table for the first symbols (see MakeFastFirst())
    bool IsAllocated, AbsOffset;
    ui32 WordMaxLength;
    TRemmapAllocation Allocation; // for fast reallocation

    ui8* GetSearchOffs() {
        return Data + TrieOffset + 1;
    }

    template <bool StopOnlyForInfoNode>
    ui8* FindNodeImpl(const ui8* input, int inpsz);

    ui8* FindNode(const ui8* input, int inpsz);

    inline int Allo(size_t sz);

    static inline void DataSetup(ui8* pos, const char* data, size_t dataSize);

    inline int SymbSetupLoop(const char* fname, int n);

    inline void SymbSetupLoopIter(const td_chr* const inp, ssize_t vsz, const char* const ldata, size_t ldatasz, int n, ui8& prevc, ui8& prevc1, ui8*& prevpos, int& ssc);

    static inline int SymbSetup(ui8* pos, td_chr c, size_t dsz1);

public:
    typedef TArrayPtr<char> TCharBuffer;

public:
    TTrieBase();
    ~TTrieBase();

    int Save(IOutputStream* f, ui32 eSig, ui32 dSig) const;
    int Load(IInputStream* f, ui32 eSig, ui32 dSig);

    int Save(const char* fname, ui32 eSig, ui32 dSig) const;
    int Load(const char* fname, ui32 eSig, ui32 dSig);

    size_t GetSizeForSave() const;

    ui8* FromMemoryMap(ui8* begin, ui32 eSig, ui32 dSig);

    void MakeFastFirst();

    inline int Reserve(size_t sz);
    /**
     * @param[in] szPos start point for search, must be pointer to sz of node.
     * @return pointer to sz of node.
     */
    inline const ui8* TreeFindNext1ByteItem(const ui8* const data, const ui8* szPos, const td_chr* input, int inpsz, int depth);

    inline ui32 GetWordMaxLength() const {
        return WordMaxLength;
    }

    ui8* GetData() const {
        return Data;
    }

    td_sz_t GetTrieOffset() const {
        return TrieOffset;
    }

    td_sz_t GetAllocationOffset() const {
        return AllocationOffset;
    }

    template <class PL>
    static inline ui32 MakeDatFiles(const char* finame, TTrieInDatFileNames& fonames,
                                    const int textField, const int dataField, const char* tmpdir);


    template <class PL>
    static inline ui32 MakeDatFilesFromYT(ICustomDataReaderPtr reader, TTrieInDatFileNames& fonames);

    TCharBuffer MakeCharBuffer() const;

    template <typename F>
    F& TrackPath(F& f) const {
        const ui8* pos = Data + TrieOffset;
        while (const td_offs_t offs = f(pos))
            pos = AbsOffset ? Data + offs : pos + offs;
        return f;
    }

    typedef TVector<std::pair<int /*strPos*/, const ui8* /*node, usable for GetLeafData*/>> TNodesVec;

    size_t FindAllNodes(const td_chr* input, int inpsz, TNodesVec& res) const;

    const ui8* FindPrefixNode(const td_chr* input, int inpsz) const;
};

template <typename N, typename PL>
class TTrie: public TTrieBase {
public:
    typedef N TNodeData;
    typedef typename PL::ValueType TLeafData;
    typedef PL TLeafDataPacker;
    typedef TTrie<N, PL> TSelf;

    static const size_t NodeDataSize, NodeDataAlignment, LeafDataSize, LeafDataAlignment;
    void AllocForSymbSetup();

    void MergeEnds(const char* tmpdir);

    void ConvertToRelOffs();

    size_t Optimize(double optimizationThresold, bool reallocate);

    size_t CalcAllNodesNum() const;

protected:
    size_t Optimize(ssize_t (*optimize)(ui8*));

    template <bool IsAbsOffset>
    size_t OptimizeImpl(ssize_t (*optimize)(ui8*));

    static ssize_t OptimizeNodeOffsets(ui8* node);
    static ssize_t OptimizeNodeData(ui8* node);

    void AllocNode(td_offs_t ptr_offs);

    static size_t HowMuch(const td_offs_t offset, td_offs_t num);

    void MergeEndsImpl(const char* tmpdir);

    int Defrag();

    size_t OptimizeNodesOffsets();

    size_t OptimizeNodesData();

public: // for service only
    static size_t GetTreenodeSize(const ui8* s);

    static TLeafData* GetLeafData(ui8* pos);

    static const TLeafData* GetLeafData(const ui8* pos) {
        return const_cast<TLeafData*>(GetLeafData((ui8*)pos));
    }

    static ui32 SimpleMakeDatFiles(const char* finame, TTrieInDatFileNames& fonames,
                                   const int textField, const int dataField, const char* tmpdir) {
        return MakeDatFiles<TLeafDataPacker>(finame, fonames, textField, dataField, tmpdir);
    }

    static ui32 SimpleMakeDatFilesFromYT(TTrieBase::ICustomDataReaderPtr reader, TTrieInDatFileNames& fonames) {
        return MakeDatFilesFromYT<TLeafDataPacker>(reader, fonames);
    }

public:
    // previously created trie files.
    // note: to load file with different/unknown L, use functions from TTrieBase
    int Save(IOutputStream* f) const {
        return TTrieBase::Save(f, N::RecordSig, PL::RecordSig);
    }
    int Load(IInputStream* f) {
        return TTrieBase::Load(f, N::RecordSig, PL::RecordSig);
    }

    int Save(const char* fname) const {
        return TTrieBase::Save(fname, N::RecordSig, PL::RecordSig);
    }
    int Load(const char* fname) {
        return TTrieBase::Load(fname, N::RecordSig, PL::RecordSig);
    }

    ui8* FromMemoryMap(ui8* begin) {
        return TTrieBase::FromMemoryMap(begin, N::RecordSig, PL::RecordSig);
    }

    void Dump(FILE* f);

    // creating new trie
    void LoadFromDatFile(const char* fname, int firstCharsCount);

    void LoadFromDatFiles(const TTrieInDatFileNames& fnames, int firstCharsCount);

    typename TNodeData::TValue Fry(const char* tmpdir, const double optimizationThresold = 0.0001);

    TLeafData* Find(const td_chr* input, int inpsz);

    const TLeafData* Find(const td_chr* input, int inpsz) const {
        return const_cast<TTrie*>(this)->Find(input, inpsz);
    }

    const TLeafData* Find(const TStringBuf& str) const { // **FIX** (use typelist.h to get right TBasicStringBuf)
        return Find((const td_chr*)str.data(), str.size());
    }

protected:
    size_t AllNodesNum;
    size_t LastOptimizedNodesNum;
};

template <typename N, typename L>
const size_t TTrie<N, L>::NodeDataSize = NTriePrivate::TypeTraits<typename TTrie<N, L>::TNodeData>::SizeOf();

template <typename N, typename L>
const size_t TTrie<N, L>::NodeDataAlignment = NTriePrivate::TypeTraits<typename TTrie<N, L>::TNodeData>::AlignOf();

// in reality we must use TSimplePacker here
template <typename N, typename L>
const size_t TTrie<N, L>::LeafDataSize = NTriePrivate::TypeTraits<typename TTrie<N, L>::TLeafData>::SizeOf();

template <typename N, typename L>
const size_t TTrie<N, L>::LeafDataAlignment = NTriePrivate::TypeTraits<typename TTrie<N, L>::TLeafData>::AlignOf();

//==============================================================================

template <typename N, typename L>
inline typename TTrie<N, L>::TLeafData* TTrie<N, L>::GetLeafData(ui8* pos) {
    td_chr sz = acr<td_chr>(pos);
    if (sz) {
        if (*pos) {
            pos += size_td_offs_info + (size_td_chr + NTriePrivate::GetSize(*pos) / 8) * sz;
        } else {
            pos += size_td_offs_info + size_td_chr_offs * sz;
            pos = ALIGN_PTR_X(size_td_offs, pos);
        }
    }
    return (TLeafData*)ALIGN_PTR_X(LeafDataAlignment, pos);
}

template <typename N, typename L>
typename TTrie<N, L>::TLeafData* TTrie<N, L>::Find(const td_chr* input, int inpsz) {
    if (ui8* pos = FindNode(input, inpsz)) {
        return GetLeafData(pos);
    } else {
        return nullptr;
    }
}

template <typename N, typename L>
void TTrie<N, L>::Dump(FILE* f) {
    using namespace NTriePrivate;
    TTreeNodesIterator<TSelf, TTreeIterItemRT> it(*this);
    TString buf;
    TLeafData* leaf;
    for (; it.p; ++it)
        if (it.BuildString(buf, leaf))
            fprintf(f, "%s\t%" PRISZT "\n", buf.data(), (size_t)*leaf);
}

template <typename N, typename PL>
size_t TTrie<N, PL>::CalcAllNodesNum() const {
    size_t result = 0;
    for (NTriePrivate::tree_nodes_iterator2<TSelf> it(*this); *it; ++it)
        result++;
    return result;
}
