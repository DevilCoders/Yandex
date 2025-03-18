#pragma once

#include "datawork.h"
#include "datawork_api.h"
#include "datawork_main.h"
#include "datawork_util.h"
#include "datawork_util.h"
#include "nullcodeproc.h"

#include <library/cpp/getopt/opt.h>
#include <library/cpp/microbdb/utility.h>

namespace NImplementationSortByLess {
    template <bool allowsort>
    struct TSorter {
        template <class TRecord>
        static int SortByLess(const TFile&, const TFile&, const TDatMetaPage*, size_t, const char*) {
            return MBDB_BAD_PARM;
        }
    };
    template <>
    struct TSorter<true> {
        template <class TRec>
        static int SortByLess(const TFile& ifile, const TFile& ofile, const TDatMetaPage* meta, size_t memory, const char* tmpDir) {
            //static_assert(false, "expect false");
            return SortData<TRec, TCompareByLess>(ifile, ofile, meta, memory, tmpDir);
        }
    };

    template <class TRec, class Tbase1, class Tbase2, bool enable>
    int SortByLess(const TFile& ifile, const TFile& ofile, const TDatMetaPage* meta, size_t memory, const char* tmpDir) {
        return TSorter<
            (enable //now detector < is not working properly
             && (std::is_same<Tbase1, TRec>::value || std::is_base_of<Tbase1, TRec>::value) && (std::is_same<Tbase2, TRec>::value || std::is_base_of<Tbase2, TRec>::value))>::template SortByLess<TRec>(ifile, ofile, meta, memory, tmpDir);
    }
}

template <typename TStructDescr>
struct TSortDispatcher {
    static const char* Rempref(const char* s) {
        const char* t = strrchr(s, ':');
        Y_VERIFY(t != nullptr, "generation error wrong typename %s\n", s);
        return t + 1;
    }

    struct TRecSortHelper {
        const TFile& Ifile;
        const TFile& Ofile;
        const TDatMetaPage* Meta;

        const char* Argv;
        size_t Mem;
        const char* TmpDir;

        int Ret;

        TRecSortHelper(const TFile& ifile, const TFile& ofile, const TDatMetaPage* m, const char* argv, size_t mem, const char* tmpDir)
            : Ifile(ifile)
            , Ofile(ofile)
            , Meta(m)
            , Argv(argv)
            , Mem(mem)
            , TmpDir(tmpDir)
            , Ret(MBDB_BAD_RECORDSIG)
        {
        }

        template <class TRec>
        struct TBySelector : TNullCodeProc {
            TRecSortHelper& Main;

            TBySelector(TRecSortHelper& m)
                : Main(m)
            {
            }

            template <int argnum, typename TAcc, typename T>
            void RegisterStaticFuncOther(const char*, T) {
            }
            template <int argnum, typename TAcc, typename T>
            void RegisterFuncOtherConst(const char*, T) {
            }

            template <typename TAcc>
            struct TSortHelper {
                template <typename TSameRec>
                struct TComparer {
                    static_assert((std::is_same<TRec, TSameRec>::value), "expect (std::is_same<TRec, TSameRec>::value)");
                    bool operator()(const TRec* l, const TRec* r) {
                        return TAcc::template Call<int, const TRec*, const TRec*>(l, r) < 0;
                    }
                    int operator()(const TRec* l, const TRec* r, int) {
                        return TAcc::template Call<int, const TRec*, const TRec*>(l, r);
                    }
                };
            };
            template <int argnum, typename TAcc, typename TRec2>
            void RegisterStaticFuncOther(const char* name, int (*func)(const TRec2*, const TRec2*)) {
                (void)func;
                static_assert(argnum == 2 && (std::is_same<TRec2, TRec>::value || std::is_base_of<TRec2, TRec>::value), "expect argnum == 2 && (std::is_same<TRec2, TRec>::value || std::is_base_of<TRec2, TRec>::value)");
                if (!stricmp(Main.Argv, name) && Main.Ret == MBDB_BAD_PARM)
                    Main.Ret = SortData<TRec, TSortHelper<TAcc>::template TComparer>(Main.Ifile, Main.Ofile, Main.Meta, Main.Mem, Main.TmpDir);
            }
            template <int argnum, typename TAcc, typename T1, typename T2>
            void RegisterFuncOtherConst(const char*, bool (T1::*)(const T2&) const) {
                if (!stricmp(Main.Argv, "-") && Main.Ret == MBDB_BAD_PARM)
                    Main.Ret = NImplementationSortByLess::SortByLess<TRec, T1, T2, (TAcc::Name0 == 15393)>(Main.Ifile, Main.Ofile, Main.Meta, Main.Mem, Main.TmpDir);
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
    static int DispatchSort(const TFile& ifile, const TFile& ofile, const TDatMetaPage* meta, const char* arg, size_t memory, const char* tmpDir) {
        TRecSortHelper r(ifile, ofile, meta, arg, memory, tmpDir);
        TStructDescr::RegisterStructs(r);
        return r.Ret;
    }
};

template <typename TDispatcher>
struct TSortWork {
    static int Work(int argc, char* argv[], TDatMetaPage* meta, TFile& ifile) {
        const char* nout = nullptr;
        const char* tmpDir = "/var/tmp/";
        TFile ofile;
        int ret, ch;
        size_t memory = 0;

        class Opt opt(argc, argv, "i:o:m:t:?");
        opt.Err = 1;
        while ((ch = opt.Get()) != EOF) {
            switch (ch) {
                case 'i':
                    break;
                case 'o':
                    nout = opt.Arg;
                    break;
                case 'm':
                    memory = atoi(opt.Arg);
                    break;
                case 't':
                    tmpDir = opt.Arg;
                    break;
                case '?':
                default:
                    return IDataworkApi::E_WRONG_USAGE;
            }
        }
        argc -= opt.Ind;
        argv += opt.Ind;
        if (argc != 1)
            return IDataworkApi::E_WRONG_USAGE;
        if (memory < 30)
            memory = 30;
        memory = memory << 20;
        OpenOutputFile(nout, ofile);
        if ((ret = TDispatcher::DispatchSort(ifile, ofile, meta, *argv, memory, tmpDir))) {
            if (ret == MBDB_BAD_PARM)
                err(1, "unknown order: \"%s", *argv);
            else if (ret != MBDB_BAD_RECORDSIG)
                err(1, "sorter error: %d", ret);
            else
                return IDataworkApi::E_NORECORD;
        }
        return IDataworkApi::AdjustRet(ret);
    }
    static void Register(IDataworkApi* api) {
        api->RegSomeFunc("sort", Work, "sort   -i <input> -o <output> -m <memory-mb> -t <temp dir> <order-func>\n");
    }
};
