#pragma once

#include <util/system/defaults.h>
#include <library/cpp/microbdb/microbdb.h>

#include "nullcodeproc.h"

namespace NImplementation {
    Y_HAS_MEMBER(RecordSig);
    template <bool v>
    struct TIndexDispatcherSupport;

    template <typename T>
    struct HackedDatfile: public TInDatFileImpl<T> {
        const T* gotopage(int pageno) {
            return TInDatFileImpl<T>::GotoPage(pageno);
        }
    };

    template <typename T, typename K>
    int IndexData(const TFile& ifile, const char* name, const TDatMetaPage* meta, size_t pagesize) {
        const size_t fbufsize = 4u << 20;

        const T* rec;
        HackedDatfile<T> datfile;
        TOutIndexFile<K> indexfile;

        if (datfile.Open(ifile, meta, fbufsize, 0))
            errx(1, "open input error %d", datfile.GetError());
        if (indexfile.Open(name, pagesize, 5))
            errx(1, "open output error %d", indexfile.GetError());

        for (int i = 0; (rec = datfile.gotopage(i)); i++) {
            K tmp = *rec;
            indexfile.Push(&tmp);
        }

        if (datfile.GetError())
            errx(1, "input error %d", datfile.GetError());
        if (datfile.Close())
            err(1, "can't close input");
        if (indexfile.GetError())
            errx(1, "output error %d", datfile.GetError());
        if (indexfile.Close())
            err(1, "can't close output");
        return 0;
    }

    template <>
    struct TIndexDispatcherSupport<false> {
        template <class T1, class T2>
        static int IndexDataV(const TFile&, const char*, const TDatMetaPage*, size_t) {
            return MBDB_BAD_PARM;
        }
    };

    template <>
    struct TIndexDispatcherSupport<true> {
        template <class TRec, class TTo>
        static int IndexDataV(const TFile& ifile, const char* name, const TDatMetaPage* meta, size_t pagesize) {
            return ::NImplementation::IndexData<TRec, TTo>(ifile, name, meta, pagesize);
        }
    };
    template <class T1, class T2>
    static int IndexDataV(const TFile& ifile, const char* name, const TDatMetaPage* meta, size_t pagesize) {
        return TIndexDispatcherSupport<THasRecordSig<T2>::value>::template IndexDataV<T1, T2>(ifile, name, meta, pagesize);
    }
    //todo special key types!
}

template <typename TStructDescr>
struct TIndexDispatcher {
    static const char* Rempref(const char* s) {
        const char* t = strrchr(s, ':');
        Y_VERIFY(t != nullptr, "generation error wrong typename %s\n", s);
        return t + 1;
    }

    struct TRecIndexerHelper {
        const TDatMetaPage* Meta;
        const TFile& Ifile;
        const char* Name;
        size_t PageSize;
        const char* Arg;

        int Ret;
        TRecIndexerHelper(const TDatMetaPage* m, const TFile& ifile, const char* name, size_t pg, const char* arg)
            : Meta(m)
            , Ifile(ifile)
            , Name(name)
            , PageSize(pg)
            , Arg(arg)
            , Ret(MBDB_BAD_RECORDSIG)
        {
        }

        template <class TRec>
        struct TBySelector : TNullCodeProc {
            TRecIndexerHelper& Main;

            TBySelector(TRecIndexerHelper& m)
                : Main(m)
            {
            }

            template <typename T>
            void RegisterBaseType(const char* name) {
                if (!stricmp(Main.Arg, Rempref(name)) && Main.Ret == MBDB_BAD_PARM)
                    Main.Ret = NImplementation::IndexDataV<TRec, T>(Main.Ifile, Main.Name, Main.Meta, Main.PageSize);
            }
            template <typename T, bool Const, bool Ref>
            void RegisterConvert(const char* name) {
                RegisterBaseType<T>(name);
            }
        };
        template <typename T, typename TInfo>
        void RegisterStruct(const char*) {
            if (T::RecordSig == Meta->RecordSig && Ret == MBDB_BAD_RECORDSIG) {
                Ret = MBDB_BAD_PARM;
                TBySelector<T> prc(*this);
                TInfo::RegisterStructMembers((T*)nullptr, prc);
            }
        }
    };
    static int DispatchIndex(const TFile& ifile, const char* name, const TDatMetaPage* meta, size_t pagesize, const char* arg) {
        TRecIndexerHelper r(meta, ifile, name, pagesize, arg);
        TStructDescr::RegisterStructs(r);
        return r.Ret;
    }
};
