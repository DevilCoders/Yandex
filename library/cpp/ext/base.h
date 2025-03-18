#pragma once

#include <functional>
#include <stdlib.h>
#include <util/folder/dirut.h>
#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/system/file.h>
#include <util/stream/output.h>
#include <util/stream/multi.h>
#include <util/stream/file.h>
#include <util/ysaveload.h>
#include <util/string/util.h>
#include "sizer.h"

template <class TDerived, class T, class TSizer = TSize<T>>
class TExtAlgorithm {
    typedef TExtAlgorithm<TDerived, T, TSizer> TSelf;

public:
    TExtAlgorithm(size_t pagesize, TSizer sizer = TSizer())
        : Size(0)
        , PageSize(pagesize)
        , Sizer(sizer)
        , TmpPrefix(FixSlash(GetSystemTempDir()))
        , Dirty(true)
        , Start(true)
    {
    }

    TExtAlgorithm(size_t pagesize, const TString& tmppref, TSizer sizer = TSizer())
        : Size(0)
        , PageSize(pagesize)
        , Sizer(sizer)
        , TmpPrefix(tmppref)
        , Dirty(true)
        , Start(true)
    {
    }

    ~TExtAlgorithm() {
        for (typename TVector<TPage*>::iterator i = Pages.begin(), ie = Pages.end(); i != ie; ++i) {
            if (*i)
                delete *i;
        }
    }

    void PushBack(const T& t) {
        Dirty = true;
        Buf.push_back(t);
        Size += Sizer(t);
        if (Size >= PageSize)
            NewPage();
    }

    void Push(const T& t) {
        PushBack(t);
    }

protected:
    class TDiskPage {
    public:
        TDiskPage(TFile f)
            : Input(f)
        {
            f.Seek(0, sSet);
            Load(&Input, Count);
            if (Count)
                Load(&Input, Current_);
        }

        bool AtEnd() const {
            return Count == 0;
        }

        const T& Current() const {
            return Current_;
        }

        void Advance() {
            --Count;
            if (Count)
                Load(&Input, Current_);
        }

    private:
        TFileInput Input;
        T Current_;
        size_t Count;
    };

    class TPodDiskPage {
    public:
        TPodDiskPage(TFile f)
            : Input(f)
            , Buf(nullptr)
            , BufLen(0)
        {
            f.Seek(0, sSet);
            Load(&Input, Count);
            DoAdvance();
        }

        bool AtEnd() const {
            return Count == 0;
        }

        const T& Current() const {
            return *Current_;
        }

        void Advance() {
            if (--Count)
                DoAdvance();
        }

    private:
        TFileInput Input;
        size_t Count;
        const T* Current_;

        const char* Buf;
        size_t BufLen;
        struct {
            char Data[sizeof(T)];
        } Placeholder;

        void DoAdvance() {
            if (BufLen == 0)
                BufLen = Input.Next(&Buf);

            if (BufLen >= sizeof(T)) {
                Current_ = reinterpret_cast<const T*>(Buf);
                Buf += sizeof(T);
                BufLen -= sizeof(T);
            } else {
                TMemoryInput remaining(Buf, BufLen);
                TMultiInput both(&remaining, &Input);
                T* p = reinterpret_cast<T*>(Placeholder.Data);
                Load(&both, *p);
                Current_ = p;
                Buf = nullptr;
                BufLen = 0;
            }
        }
    };

public:
    typedef T TValue;
    typedef std::conditional_t<TTypeTraits<T>::IsPod, TPodDiskPage, TDiskPage> TPage;

    void Swap(TSelf& rhs) {
        DoSwap(Pages, rhs.Pages);
        DoSwap(Current_, rhs.Current_);
        DoSwap(Size, rhs.Size);
        DoSwap(PageSize, rhs.PageSize);
        DoSwap(Buf, rhs.Buf);
        DoSwap(Files, rhs.Files);
        DoSwap(Sizer, rhs.Sizer);
        DoSwap(TmpPrefix, rhs.TmpPrefix);
        DoSwap(Dirty, rhs.Dirty);
        DoSwap(Start, rhs.Start);
    }

    void Rewind() {
        for (typename TVector<TPage*>::iterator i = Pages.begin(), ie = Pages.end(); i != ie; ++i) {
            if (*i)
                delete *i;
        }
        Pages.erase(Pages.begin(), Pages.end());
        if (!Buf.empty())
            NewPage();
        for (TVector<TFile>::iterator i = Files.begin(), ie = Files.end(); i != ie; ++i)
            Pages.push_back(new TPage(*i));
        Current_ = Pages.begin();
        static_cast<TDerived*>(this)->DoRewind();
        Dirty = false;
        Start = true;
    }

    const T& Current() const {
        return (*Current_)->Current();
    }

    void Advance() {
        static_cast<TDerived*>(this)->DoAdvance();
    }

    bool AtEnd() const {
        return (Current_ == Pages.end());
    }

    const T* Next() {
        if (Dirty)
            Rewind();
        else if (!Start)
            Advance();
        Start = false;
        return AtEnd() ? nullptr : &Current();
    }

    void SetTmpPrefix(const TString& tmppref) {
        TmpPrefix = tmppref;
    }

    size_t GetPageSize() const {
        return PageSize;
    }

    const TString& GetTmpPrefix() const {
        return TmpPrefix;
    }

public:
    TVector<TPage*> Pages;
    typename TVector<TPage*>::iterator Current_;

private:
    size_t Size;
    size_t PageSize;
    TVector<T> Buf;
    TVector<TFile> Files;
    TSizer Sizer;
    TString TmpPrefix;
    bool Dirty;
    bool Start;

    void NewPage() {
        TFile f = TFile::Temporary(TmpPrefix); // file is counting refs to the handle
        static_cast<TDerived*>(this)->PreSave(Buf);

        TFixedBufferFileOutput out(f);
        Save(&out, Buf.size());
        if (!Buf.empty())
            SaveArray(&out, &Buf[0], Buf.size());
        out.Finish();

        Buf.erase(Buf.begin(), Buf.end());
        Files.push_back(f);

        Size = 0;
    }

    inline static TString FixSlash(TString path) {
        if (!path.EndsWith(LOCSLASH_S))
            path.append(LOCSLASH_C);
        return path;
    }
};
