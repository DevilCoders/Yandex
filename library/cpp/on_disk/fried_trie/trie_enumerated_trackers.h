#pragma once

#include "trie_enumerated.h"

#include <typeinfo>

namespace EIdToStringTrackerPrivate {
    struct TPCharWrapper {
        typedef char* TReference;
        TReference Buf, Pos;
        TPCharWrapper(TReference buf)
            : Buf(buf)
            , Pos(buf)
        {
        }
        inline void Append(td_chr c) {
            *Pos++ = c;
        }
        inline void Finish() {
            *Pos = '\0';
        }
    };

    struct TStrokaWrapper {
        typedef TString& TReference;
        TReference Buf;
        TStrokaWrapper(TReference buf)
            : Buf(buf)
        {
        }
        inline void Append(td_chr c) {
            Buf.append(c);
        }
        inline void Finish() {
        }
    };
}

template <typename T, typename W>
struct TIdToStringTracker {
    typedef T TTrie;
    typedef W TBufferWrapper;
    typedef typename TBufferWrapper::TReference TBufferReference;
    typedef TLeafCounter::TValue TValue;
    TValue Id;
    TBufferWrapper BufferWrapper;

    TIdToStringTracker(TValue id, TBufferReference buf)
        : Id(id)
        , BufferWrapper(buf)
    {
        Y_ASSERT(Id > 0);
        Y_ASSERT(typeid(typename TTrie::TNodeData) == typeid(TLeafCounter));
    }

    td_offs_t operator()(const ui8* pos) {
        using namespace NTriePrivate;
        const td_chr l = acr<td_chr>(pos);
        if (!(l & td_info_flag))
            if (!--Id) {
                BufferWrapper.Finish();
                return 0;
            }
        Y_ASSERT(Id > 0);
        const td_chr sz = acr<td_chr>(pos);
        if (!sz) {
            //            *Buf = '\0';
            return 0;
        }
        const ui8 offs_info = acr<ui8>(pos);
        const ui8 offs_type = GetType(offs_info), offs_size = GetSize(offs_info);
        const ui8* const chrs = pos;
        pos += sz * size_td_chr;
        const ui8* const offs = pos;
        if (sz == 1) {
            BufferWrapper.Append(chrs[0]);
            return Decode<td_offs_t>(offs_type, offs_size, pos, 0);
        }
        Y_ASSERT(offs_info);
        pos = ALIGN_PTR_X(TTrie::LeafDataAlignment, pos + (sz * offs_size / 8) + TTrie::LeafDataSize);
        Y_ASSERT(*pos);
        const size_t found = TLeafCounter::Search(pos, sz, Id - 1);
        //        printf("AAA search(%u, %u) = %u\n", sz, Id, found); return 0;
        if (found)
            Id -= Decode<TValue>(*pos, pos + sizeof(ui8), found - 1);
        BufferWrapper.Append(chrs[found]);
        return Decode<td_offs_t>(offs_type, offs_size, offs, found);
    }
};

template <typename T>
struct TIdToPCharTracker: public TIdToStringTracker<T, EIdToStringTrackerPrivate::TPCharWrapper> {
    typedef TIdToStringTracker<T, EIdToStringTrackerPrivate::TPCharWrapper> TSuper;
    TIdToPCharTracker(typename TSuper::TValue id, typename TSuper::TBufferReference buf)
        : TSuper(id, buf)
    {
    }
};

template <typename T>
struct TIdToStrokaTracker: public TIdToStringTracker<T, EIdToStringTrackerPrivate::TStrokaWrapper> {
    typedef TIdToStringTracker<T, EIdToStringTrackerPrivate::TStrokaWrapper> TSuper;
    TIdToStrokaTracker(typename TSuper::TValue id, typename TSuper::TBufferReference buf)
        : TSuper(id, buf)
    {
    }
};

template <typename T, typename TpDeriv>
struct TStringToIdTrackerBase {
    typedef T TTrie;
    typedef TLeafCounter::TValue TValue;
    TValue Id;
    const td_chr* const Input;

protected:
    TStringToIdTrackerBase(const td_chr* input)
        : Id(0)
        , Input(input)
        , Pos(Input)
    {
        Y_ASSERT(typeid(typename TTrie::TNodeData) == typeid(TLeafCounter));
    }
    TStringToIdTrackerBase(const char* input)
        : Id(0)
        , Input((const td_chr*)input)
        , Pos(Input)
    {
        Y_ASSERT(typeid(typename TTrie::TNodeData) == typeid(TLeafCounter));
    }

public:
    td_offs_t operator()(const ui8* pos) {
        using namespace NTriePrivate;
        const td_chr l = acr<td_chr>(pos);
        const bool next = 0 == (l & td_info_flag);
        if (next)
            ++Id;
        if (static_cast<TpDeriv*>(this)->Done()) {
            if (!next)
                Id = 0;
            return 0;
        }
        const td_chr sz = acr<td_chr>(pos);
        const ui8 offs_info = sz ? acr<ui8>(pos) : 0;
        const td_chr* start = (td_chr*)pos;
        pos += sz * sizeof(td_chr);
        const ui8* const offs = pos;
        const td_chr* found = mybinsearch(start, (td_chr*)pos, *Pos++);
        if (!found) {
            Id = 0;
            return 0;
        }
        const ui8 offs_type = GetType(offs_info), offs_size = GetSize(offs_info);
        if (offs_size) {
            pos += sz * offs_size / 8;
        } else {
            pos = ALIGN_PTR_X(size_td_offs, pos) + sz * size_td_offs;
        }
        pos = ALIGN_PTR_X(TTrie::LeafDataAlignment, pos + TTrie::LeafDataSize);
        if (found - start)
            Id += Decode<TValue>(*pos, pos + sizeof(ui8), found - start - 1);
        return NTriePrivate::Decode<td_offs_t>(offs_type, offs_size, offs, found - start);
    }

protected:
    const td_chr* Pos;
};

template <typename T>
struct TStringToIdTracker
   : public TStringToIdTrackerBase<T, TStringToIdTracker<T>> {
    typedef TStringToIdTrackerBase<T, TStringToIdTracker> TdBase;
    TStringToIdTracker(const td_chr* input)
        : TdBase(input)
    {
    }
    TStringToIdTracker(const char* input)
        : TdBase(input)
    {
    }
    bool Done() const {
        return !*TdBase::Pos;
    }
};

template <typename T>
struct TStringBufToIdTracker
   : public TStringToIdTrackerBase<T, TStringBufToIdTracker<T>> {
    typedef TStringToIdTrackerBase<T, TStringBufToIdTracker> TdBase;
    TStringBufToIdTracker(const TStringBuf& str)
        : TdBase(str.data())
        , End_(TdBase::Pos + str.length())
    {
    }
    bool Done() const {
        return TdBase::Pos == End_;
    }

private:
    const td_chr* const End_;
};
