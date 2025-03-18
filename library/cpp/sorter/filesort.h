#pragma once

#include "sorttree.h"

#include <util/system/event.h>
#include <util/system/thread.h>
#include <util/system/mutex.h>
#include <util/system/maxlen.h>
#include <util/system/file.h>
#include <util/system/filemap.h>
#include <util/system/yassert.h>
#include <util/system/defaults.h>
#include <util/stream/file.h>
#include <util/string/printf.h>
#include <util/generic/string.h>
#include <util/generic/algorithm.h>

class CFileSortBase {
protected:
    TString m_prefix;
    size_t m_maxElements;
    size_t m_curSort;
    size_t m_maxsSort;

    CFileSortBase()
        : m_maxElements(0)
        , m_curSort(0)
        , m_maxsSort(0)
    {
    }

    void Open(const char* prefix, size_t maxElements) {
        m_prefix = prefix;
        m_maxElements = maxElements;
    }

    TFile OpenPortion(size_t seq_no) {
        TString name = Sprintf("%s.%" PRISZT, m_prefix.data(), seq_no);
        return TFile(name, RdWr | CreateAlways | AWUser | ARUser | AWGroup | ARGroup | AROther);
    }

    TFile ReadyPortion(size_t seq_no) {
        TString name = Sprintf("%s.%" PRISZT, m_prefix.data(), seq_no);
        return TFile(name, RdOnly);
    }
};

template <class C, class F, class CF = C>
class CFileSort: public CFileSortBase {
public:
    CSortTree<CF, F> m_sortTree;

protected:
    bool m_sortFinish;
    TSystemEvent m_qSortEvent;
    TSystemEvent m_aSortEvent;
    TMappedAllocation m_CurrentMap;
    TMappedAllocation m_SortedMap;
    TThread m_sortThread;

public:
    CFileSort()
        : m_sortFinish(false)
        , m_qSortEvent(TSystemEvent::rAuto)
        , m_aSortEvent(TSystemEvent::rAuto)
        , m_sortThread(SortProc, this)
    {
    }

    virtual ~CFileSort() {
        if (!m_sortFinish) {
            DoFinish();
        }
        m_CurrentMap.Dealloc();
        m_SortedMap.Dealloc();
    }

    void Open(const char* prefix, size_t maxElements) {
        CFileSortBase::Open(prefix, maxElements);
        m_CurrentMap.Alloc(m_maxElements * sizeof(C));
        m_sortThread.Start();
    }

    void SaveCount() {
        TString name = Sprintf("%s.count", m_prefix.data());
        TFile f(name, OpenAlways | WrOnly);

        ui32 size = m_sortTree.size();
        f.Write(&size, sizeof(size));
    }

    void OpenSorted(const char* prefix) {
        ui32 size;
        m_prefix = prefix;
        TString name = Sprintf("%s.count", m_prefix.data());

        TFile f(name, OpenExisting | RdOnly);
        f.Load(&size, sizeof(size));

        m_sortTree.resize(size);
        for (ui32 i = 0; i < size; i++)
            m_sortTree[i].m_file = ReadyPortion(i);
        m_sortTree.Init();
    }

    void SortPiece() {
        m_aSortEvent.Wait();
        m_SortedMap.swap(m_CurrentMap);
        m_maxsSort = m_curSort;
        m_qSortEvent.Signal();
        m_curSort = 0;
        Y_ASSERT(m_CurrentMap.Ptr() == nullptr);
    }

    void AddElement(const C& elem) {
        C* buffer = (C*)m_CurrentMap.Ptr();
        new (&buffer[m_curSort]) C();
        buffer[m_curSort++] = elem;
        if (m_curSort >= m_maxElements) {
            SortPiece();
            m_CurrentMap.Alloc(m_maxElements * sizeof(C));
        }
    }

    void Finish() {
        SortPiece();
        m_aSortEvent.Wait();
        DoFinish();
        m_sortTree.Init();
    }

    void Reset() {
        m_sortTree.Init();
    }

private:
    void DoFinish() {
        m_sortFinish = true;
        m_qSortEvent.Signal();
        m_sortThread.Join();
    }

    // sorting thread's routines
    static void* SortProc(void* param) {
        try {
            CFileSort<C, F, CF>* p = (CFileSort<C, F, CF>*)param;
            p->SortThread();
        } catch (const yexception& e) {
            fprintf(stderr, "Exception in sorter thread : %s\n", e.what());
            throw;
        } catch (...) {
            fprintf(stderr, "Unknown exception in sorter thread\n");
            throw;
        }
        return nullptr;
    }

    void SortThread() {
        m_aSortEvent.Signal();
        do {
            m_qSortEvent.Wait();
            if (m_SortedMap.Ptr()) {
                if (m_maxsSort) {
                    SortAndWritePiece();
                }
                m_SortedMap.Dealloc();
            }
            m_aSortEvent.Signal();
        } while (!m_sortFinish);
    }

protected:
    virtual void SortAndWritePiece() {
        C* slinks = (C*)m_SortedMap.Ptr();
        Sort(slinks, slinks + m_maxsSort);
        size_t sz = m_sortTree.size();
        m_sortTree.AppendSource();
        m_sortTree.back().m_file = OpenPortion(sz);
        WritePiece(m_sortTree.back().m_file);
    }
    virtual void WritePiece(TFile& f) {
        f.Write(m_SortedMap.Ptr(), sizeof(C) * m_maxsSort);
    }
};

template <class C, class F, class CF = C>
class CFileSortDirectWrite: public CFileSort<C, F, CF> {
public:
    using CFileSort<C, F, CF>::m_maxsSort;
    using CFileSort<C, F, CF>::m_SortedMap;

public:
    virtual void WritePiece(TFile& f) {
        f.SetDirect();
        ui32 len = sizeof(C) * m_maxsSort;
        f.Write(m_SortedMap.Ptr(), len & ~511);
        f.ResetDirect();
        if (len & 511)
            f.Write((char*)m_SortedMap.Ptr() + (len & ~511), len & 511);
    }
};

template <class C, class F, class CF = C>
class CFileSortMut: public CFileSort<C, F, CF> {
public:
    TMutex* Mutex;

public:
    CFileSortMut(TMutex* mutex)
        : Mutex(mutex)
    {
    }

    void WritePiece(TFile& f) override {
        if (Mutex)
            Mutex->Acquire();
        CFileSort<C, F, CF>::WritePiece(f);
        if (Mutex)
            Mutex->Release();
    }
};

template <class C, class F, template <class C1, class F1, class CF1 = C1> class S = CFileSort>
class CFileSortU: public S<C, F> {
protected:
    ui32 UniqSorted() {
        C* start = (C*)S<C, F>::m_SortedMap.Ptr();
        C* finish = start + S<C, F>::m_maxsSort;
        C* d = start;
        for (C* p = start; p < finish;) {
            if (p > d)
                *d = *p;

            d++;
            C* pp = p;

            while ((p < finish) && (*p == *pp)) {
                p++;
            }
        }
        return d - start;
    }

    virtual void WritePiece(TFile& f) override {
        ui32 len = UniqSorted();
        f.Write(S<C, F>::m_SortedMap.Ptr(), len * sizeof(C));
    }
};

template <class C, class F, template <class C1, class F1, class CF1 = C1> class S = CFileSort>
class CFileSortDirectWriteU: public CFileSortU<C, F, S> {
public:
    using CFileSortU<C, F, S>::m_SortedMap;
    using CFileSortU<C, F, S>::UniqSorted;

protected:
    virtual void WritePiece(TFile& f) override {
        ui32 count = UniqSorted();
        f.SetDirect();
        ui32 len = sizeof(C) * count;
        f.Write(m_SortedMap.Ptr(), len & ~511);
        f.ResetDirect();
        if (len & 511)
            f.Write((char*)m_SortedMap.Ptr() + (len & ~511), len & 511);
    }
};

template <class C, class F, template <class C1, class F1, class CF1 = C1> class S = CFileSort>
class CFileSortUS: public S<C, F> {
protected:
    void WritePiece(TFile& f) override {
        C* start = (C*)S<C, F>::m_SortedMap.Ptr();
        C* finish = start + S<C, F>::m_maxsSort;
        C* d = start;
        for (C* p = start; p < finish;) {
            if (p > d)
                *d = *p;
            C* pp = p;
            p++;

            while ((p < finish) && (*p == *pp)) {
                *d += *p++;
            }

            d++;
        }
        f.Write(start, (d - start) * sizeof(C));
    }
};
