#pragma once

#include "basetypestdio.h"
#include "datawork.h"
#include "datawork_dump.h"
#include "datawork_main.h"
#include "datawork_api.h"
#include "datawork_util.h"
#include "nullcodeproc.h"
#include <google/protobuf/text_format.h>

template <typename TStructDescr>
struct TDumpCountDispatcher {
    template <typename T>
    static int CountData(const TFile& ifile, const TFile& ofile, const TDatMetaPage* meta, int /*argc*/, char** /*argv*/) {
        const size_t recsize = sizeof(T);
        const size_t bufsize = 4 * recsize + 1;
        const size_t fbufsize = 4u << 20;

        const T* rec;
        char* buf;
        TInDatFileImpl<T> datfile;

        if (!(buf = (char*)malloc(bufsize)))
            err(1, "can't allocate %" PRISZT " bytes", bufsize);
        TOFStream foutput(ofile, fbufsize);
        if (datfile.Open(ifile, meta, fbufsize, 0))
            errx(1, "data file error %d", datfile.GetError());

        long count = 0;
        while ((rec = datfile.Next())) {
            ++count;
        }
        foutput << count << Endl;

        if (datfile.GetError())
            errx(1, "input error %d", datfile.GetError());
        if (datfile.Close())
            err(1, "can't close input");
        return 0;
    }

    static const char* Rempref(const char* s) {
        const char* t = strrchr(s, ':');
        Y_VERIFY(t != nullptr, "generation error wrong typename %s\n", s);
        return t + 1;
    }

    struct TDumpRecordSimple : TNullCodeProc {
        char* To;
        TDumpRecordSimple(char* to)
            : To(to)
        {
        }
        template <class T>
        void Dump(const T& f) {
            WriteField(f, To);
            *To++ = '\t';
        }
        template <typename TAcc, typename T>
        void RegisterOrdinal(const char*, T& f) {
            Dump(f);
        }
        template <int n, typename TAcc>
        void RegisterConstBitfield(const char*, i64 f) {
            Dump(f);
        }
        template <int n, typename TAcc>
        void RegisterBitfield(const char*, i64 f) {
            Dump(f);
        }
        template <typename TAcc, typename T>
        void RegisterConst(const char*, const T& f) {
            Dump(f);
        }
    };
    template <typename TRec>
    struct TDumpRecordComplexBase : TNullCodeProc {
        typedef void (*TDumpFieldTo)(const TRec& f, char*& to);

        virtual void Set(const char* n, TDumpFieldTo fun) = 0;
        template <class T>
        struct TSetPtr {
            const T& Ref;
            TSetPtr(const T& r)
                : Ref(r)
            {
            } //better todo this via ptr but some one used auto_zero in base class (TDataworkDate)
        };
        template <class TAcc, class T>
        static void DumpOrdinal(const TRec& f, char*& to) {
            WriteField(TAcc::template Get<TSetPtr<T>>(f).Ref, to); //without copy (inplace)
            *to++ = '\t';
        }
        template <typename TAcc, typename T>
        void RegisterOrdinal(const char* n, T&) {
            Set(n, &DumpOrdinal<TAcc, T>);
        }
        template <typename TAcc, typename T>
        void RegisterConst(const char* n, const T&) {
            Set(n, &DumpOrdinal<TAcc, T>);
        }

        //todo ! RegisterStaticFuncOther<2> for sorters
        //RegisterConst, RegisterConstBitfield, RegisterBitfield
        template <class TAcc, class T>
        static void DumpFunc(const TRec& f, char*& to) {
            WriteField(TAcc::template Call<T>(f), to);
            *to++ = '\t';
        }
        template <typename TAcc, typename T>
        void RegisterFuncConst(const char*, void (T::*)() const) {
        }
        //may be better this header ???
        template <typename TAcc, typename T, typename TR>
        void RegisterFuncConst(const char* n, TR (T::*)() const)
        //template<typename TAcc, typename TR> void RegisterFuncConst(const char* n,  TR (TRec::*)() const)  //visual studio accepts with (promoting function to all inherited)
        {
            Set(n, &DumpFunc<TAcc, TR>);
        }
        template <typename TAcc, typename TR>
        void RegisterFuncConst(const char* name, TR (TRec::*f)() const) {
            RegisterFuncConst<TAcc, TRec, TR>(name, f);
        }

        template <class TAcc>
        static void DumpBitField(const TRec& f, char*& to) {
            WriteField(TAcc::template Get<i64>(f), to);
            *to++ = '\t';
        }
        template <int n, typename TAcc>
        void RegisterConstBitfield(const char* s, i64) {
            Set(s, &DumpBitField<TAcc>);
        }
        template <int n, typename TAcc>
        void RegisterBitfield(const char* s, i64) {
            Set(s, &DumpBitField<TAcc>);
        }

        virtual ~TDumpRecordComplexBase() {
        }
    };
    template <typename TRec>
    struct TDumpRecordComplex : TDumpRecordComplexBase<TRec> {
        typedef typename TDumpRecordComplexBase<TRec>::TDumpFieldTo TDumpFieldTo;
        TVector<TDumpFieldTo> DumpFuncs;
        TVector<std::pair<TString, int>> Opts;

        TDumpRecordComplex(int argc, char* argv[]) {
            for (int i = 0; i < argc; i++) {
                Opts.push_back(std::make_pair(TString(argv[i]), i));
                Opts.back().first.to_lower();
            }
            ::Sort(Opts.begin(), Opts.end());
            DumpFuncs.resize(Opts.size(), (TDumpFieldTo) nullptr);
        }
        void Dump(const TRec& rec, char* to) {
            char* was = to;
            for (typename TVector<TDumpFieldTo>::const_iterator i = DumpFuncs.begin(); i != DumpFuncs.end(); ++i)
                (**i)(rec, to);
            if (to > was)
                --to; //we already printed '\t'
            *to = '\0';
        }
        bool AllFilled() const {
            for (typename TVector<TDumpFieldTo>::const_iterator i = DumpFuncs.begin(); i != DumpFuncs.end(); ++i)
                if (*i == nullptr)
                    return false;
            return true;
        }

        void Set(const char* n, TDumpFieldTo fun) override {
            TString sn(n);
            sn.to_lower();
            TVector<std::pair<TString, int>>::const_iterator i = ::LowerBound(Opts.begin(), Opts.end(), std::make_pair(sn, (int)0));
            for (; i != Opts.end() && i->first == sn; ++i)
                DumpFuncs[i->second] = fun;
        }
    };
    struct TDumpRecordMain {
        const TDatMetaPage* Meta;
        const TFile& Ifile;
        const TFile& Ofile;
        int Argc;
        char** Argv;
        int Ret;
        TDumpRecordMain(const TDatMetaPage* m, const TFile& ifile, const TFile& ofile, int argc, char* argv[])
            : Meta(m)
            , Ifile(ifile)
            , Ofile(ofile)
            , Argc(argc)
            , Argv(argv)
            , Ret(MBDB_BAD_RECORDSIG)
        {
        }

        template <typename T, typename TInfo>
        int DumpData() {
            const size_t recsize = sizeof(T);
            const size_t bufsize = 4 * recsize + 1;
            const size_t fbufsize = 4u << 20;

            const T* rec = nullptr;
            typename TExtInfoType<T>::TResult extInfo;
            char* buf;
            TInDatFileImpl<T> datfile;

            if (!(buf = (char*)malloc(bufsize)))
                err(1, "can't allocate %" PRISZT " bytes", bufsize);

            TOFStream foutput(Ofile, fbufsize);

            if (datfile.Open(Ifile, Meta, fbufsize, 0))
                errx(1, "data file error %d", datfile.GetError());

            google::protobuf::TextFormat::Printer printer;
            printer.SetSingleLineMode(true);
            TString dump;
            bool dumpExtInfo = false;
            if (Argc && !stricmp(Argv[Argc - 1], "extInfo")) {
                if (!TExtInfoType<T>::Exists)
                    errx(1, "no extInfo for this type");
                dumpExtInfo = true;
                --Argc;
            }
            if (Argc) {
                TDumpRecordComplex<T> optfiller(Argc, Argv);
                TInfo::RegisterStructMembers(const_cast<T*>(rec), optfiller);
                if (!optfiller.AllFilled())
                    errx(1, "bad fields");

                //int *fields = (int*)alloca(sizeof(int)*(argc+1));
                //if (ParseDumpArgs<T>(argc, argv, fields))
                //  errx(1, "bad fields");
                while ((rec = datfile.Next())) {
                    optfiller.Dump(*rec, buf);
                    foutput.Write(buf);
                    if (dumpExtInfo) {
                        datfile.GetExtInfo(&extInfo);
                        printer.PrintToString(extInfo, &dump);
                        if (Argc)
                            foutput.Write("\t");
                        foutput.Write(dump.data());
                    }
                    foutput.Write("\n");
                }
            } else {
                while ((rec = datfile.Next())) {
                    if (SizeOf(rec)) {
                        TDumpRecordSimple dmp(buf);
                        TInfo::RegisterStructMembers(const_cast<T*>(rec), dmp);
                        if (dmp.To > buf)
                            --dmp.To; //we already printed '\t'
                        *dmp.To = '\0';
                    } else
                        buf[0] = '\0';
                    foutput.Write(buf);
                    if (TExtInfoType<T>::Exists) {
                        datfile.GetExtInfo(&extInfo);
                        printer.PrintToString(extInfo, &dump);
                        if (SizeOf(rec))
                            foutput.Write("\t");
                        foutput.Write(dump.data());
                    }
                    foutput.Write("\n");
                }
            }

            if (datfile.GetError())
                errx(1, "input error %d", datfile.GetError());
            if (datfile.Close())
                err(1, "can't close input");
            free(buf);
            return 0;
        }
        template <typename T, typename TInfo>
        void RegisterStruct(const char*) {
            if (T::RecordSig == Meta->RecordSig && Ret == MBDB_BAD_RECORDSIG)
                Ret = DumpData<T, TInfo>();
        }
    };
    static int DispatchDump(const TFile& ifile, const TFile& ofile, const TDatMetaPage* meta, int argc, char* argv[]) {
        TDumpRecordMain r(meta, ifile, ofile, argc, argv);
        TStructDescr::RegisterStructs(r);
        return r.Ret;
    }

    struct TCountRecordMain {
        const TDatMetaPage* Meta;
        const TFile& Ifile;
        const TFile& Ofile;
        int Argc;
        char** Argv;
        int Ret;
        TCountRecordMain(const TDatMetaPage* m, const TFile& ifile, const TFile& ofile, int argc, char* argv[])
            : Meta(m)
            , Ifile(ifile)
            , Ofile(ofile)
            , Argc(argc)
            , Argv(argv)
            , Ret(MBDB_BAD_RECORDSIG)
        {
        }

        template <typename T, typename TInfo>
        void RegisterStruct(const char*) {
            if (T::RecordSig == Meta->RecordSig && Ret == MBDB_BAD_RECORDSIG)
                Ret = CountData<T>(Ifile, Ofile, Meta, Argc, Argv);
        }
    };
    static int DispatchCount(const TFile& ifile, const TFile& ofile, const TDatMetaPage* meta, int argc, char* argv[]) {
        TCountRecordMain r(meta, ifile, ofile, argc, argv);
        TStructDescr::RegisterStructs(r);
        return r.Ret;
    }
};
