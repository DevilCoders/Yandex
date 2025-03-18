#pragma once

#include "datawork.h"
#include "datawork_main.h"
#include "datawork_api.h"
#include "datawork_filter.h"
#include "datawork_util.h"
#include "nullcodeproc.h"
#include "datawork_dump.h"

template <typename TStructDescr>
struct TFilterDispatcherScope {
    template <class T, class TCore, class TSel>
    static int FilterData(const TFile& ifile, const TFile& ofile, const TDatMetaPage* meta, TCore& core, const TSel& sel) {
        const T* rec = nullptr;
        TInDatFileImpl<T> idatfile;
        size_t inp_buf = std::max(2 * (size_t)meta->PageSize, (size_t)TCore::InputBuffer);
        if (idatfile.Open(ifile, meta, inp_buf, 0))
            errx(1, "data file error %d", idatfile.GetError());

        TOutDatFileImpl<T> odatfile;

        size_t pages = std::max((size_t)2u, TCore::OutputBuffer / idatfile.GetPageSize());

        if (odatfile.Open(ofile, idatfile.GetPageSize(), pages))
            err(1, "can't write outout");

        core(rec, idatfile, odatfile, sel);

        if (idatfile.GetError())
            err(1, "input error %d", idatfile.GetError());
        if (idatfile.Close())
            err(1, "can't close input");
        if (odatfile.GetError())
            err(1, "output error %d", odatfile.GetError());
        if (odatfile.Close())
            err(1, "can't close output");
        return 0;
    }

    template <class TFld>
    struct TFieldSelector {
        TFieldSelector(const TFld& fld) {
            dif = (ptrdiff_t)(&fld);
        } //of 0 ptr rec

        ptrdiff_t dif;

        template <class TCurRec, class Toper>
        typename Toper::TRet operator()(const TCurRec* rec, Toper& op) const {
            return op(rec, *(const TFld*)((ui8*)rec + dif));
        }
    };

    template <class TRec, class TCore, class TFld>
    static int RedirectFilterData(const TFile& ifile, const TFile& ofile, const TDatMetaPage* meta, TCore& core, const TFld& fld) {
        return FilterData<TRec, TCore, TFieldSelector<TFld>>(ifile, ofile, meta, core, TFieldSelector<TFld>(fld));
    }

    template <class TCore>
    struct TFilterDispatcher {
        static const char* Rempref(const char* s) {
            const char* t = strrchr(s, ':');
            Y_VERIFY(t != nullptr, "generation error wrong typename %s\n", s);
            return t + 1;
        }

        struct TRecFilterHelper {
            const TFile& Ifile;
            const TFile& Ofile;
            const TDatMetaPage* Meta;

            char* Argv;
            TCore& Core;

            int Ret;

            TRecFilterHelper(const TFile& ifile, const TFile& ofile, const TDatMetaPage* m, char* argv, TCore& c)
                : Ifile(ifile)
                , Ofile(ofile)
                , Meta(m)
                , Argv(argv)
                , Core(c)
                , Ret(MBDB_BAD_RECORDSIG)
            {
            }

            template <class TRec>
            struct TBySelector : TNullCodeProc {
                TRecFilterHelper& Main;

                TBySelector(TRecFilterHelper& m)
                    : Main(m)
                {
                }

                template <typename TAcc, typename T>
                void RegisterOrdinal(const char* name, T& val) {
                    if (!stricmp(Main.Argv, name) && Main.Ret == MBDB_BAD_PARM)
                        Main.Ret = RedirectFilterData<TRec, TCore>(Main.Ifile, Main.Ofile, Main.Meta, Main.Core, val); //val is null ptr offset
                }
                template <typename TAcc, typename T>
                void RegisterConst(const char* name, const T& val) {
                    if (!stricmp(Main.Argv, name) && Main.Ret == MBDB_BAD_PARM)
                        Main.Ret = RedirectFilterData<TRec, TCore>(Main.Ifile, Main.Ofile, Main.Meta, Main.Core, val); //val is null ptr offset
                }

                //tood RegisterConst, RegisterConst, RegisterBitfield

                template <class TAccessor, class TRet>
                struct TFuncSelector { //TODO FASTER COMPILING MAKE VIRTUAL CALL HERE (CALL BY PTR)
                    template <class Toper>
                    typename Toper::TRet operator()(const TRec* rec, Toper& op) const {
                        return op(rec, TAccessor::template Call<TRet>(*rec));
                    }
                };

                template <class TAccessor, class TRet>
                struct TBitField { //TODO FASTER COMPILING MAKE VIRTUAL CALL HERE (CALL BY PTR)
                    template <class Toper>
                    typename Toper::TRet operator()(const TRec* rec, Toper& op) const {
                        return op(rec, TAccessor::template Get<TRet>(*rec));
                    }
                };
                template <int n, typename TAcc>
                void RegisterConstBitfield(const char* name, i64) {
                    if (!stricmp(Main.Argv, name) && Main.Ret == MBDB_BAD_PARM)
                        if (n < 32)
                            Main.Ret = FilterData<TRec, TCore, TBitField<TAcc, i32>>(Main.Ifile, Main.Ofile, Main.Meta, Main.Core, TBitField<TAcc, i32>());
                        else
                            Main.Ret = FilterData<TRec, TCore, TBitField<TAcc, i64>>(Main.Ifile, Main.Ofile, Main.Meta, Main.Core, TBitField<TAcc, i64>());
                }
                template <int n, typename TAcc>
                void RegisterBitfield(const char* name, i64) {
                    RegisterConstBitfield<n, TAcc>(name, 0);
                }

                template <typename TAcc, typename T>
                void RegisterFuncConst(const char*, void (T::*)() const) {
                }

                template <typename TAcc, typename T, typename TR>
                void RegisterFuncConst(const char* name, TR (T::*)() const) {
                    if (!stricmp(Main.Argv, name) && Main.Ret == MBDB_BAD_PARM)
                        Main.Ret = FilterData<TRec, TCore, TFuncSelector<TAcc, TR>>(Main.Ifile, Main.Ofile, Main.Meta, Main.Core, TFuncSelector<TAcc, TR>());
                }
                template <typename TAcc, typename TR>
                void RegisterFuncConst(const char* name, TR (TRec::*f)() const) {
                    RegisterFuncConst<TAcc, TRec, TR>(name, f);
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
        static int DispatchFilter(const TFile& ifile, const TFile& ofile, const TDatMetaPage* meta, char* argv, TCore& core) {
            TRecFilterHelper r(ifile, ofile, meta, argv, core);
            TStructDescr::RegisterStructs(r);
            return r.Ret;
        }
    };
};
