#pragma once

#include "datawork.h"
#include "datawork_main.h"
#include "datawork_api.h"
#include "datawork_util.h"
#include "datawork_load.h"

#include "nullcodeproc.h"
#include <library/cpp/microbdb/extinfo.h>

#include "basetypestdio.h"
#include <google/protobuf/descriptor.h>

template <typename TStructDescr>
struct TLoadInfoDispatcher {
    static const char* Rempref(const char* s) {
        const char* t = strrchr(s, ':');
        Y_VERIFY(t != nullptr, "generation error wrong typename %s\n", s);
        return t + 1;
    }

    static inline void WritePbInfo(const google::protobuf::Descriptor* d) {
        printf("ExtInfo: (%s) %s", d->file()->name().data(), d->DebugString().data());
    }

    struct TDumpRecord : TNullCodeProc {
        ui32 Recordsig;
        bool Found;
        TDumpRecord(ui32 r)
            : Recordsig(r)
            , Found(false)
        {
        }
        template <typename TAcc, typename T>
        void RegisterOrdinal(const char* s, T&) {
            printf("%s::%d\n", s, (int)sizeof(T));
        }
        template <typename TAcc, typename T>
        void RegisterConst(const char* s, const T&) {
            printf("%s::%d const\n", s, (int)sizeof(T));
        }
        template <int n, typename TAcc>
        void RegisterBitfield(const char* s, i64) {
            printf("%s::.%d\n", s, n);
        }
        template <int n, typename TAcc>
        void RegisterConstBitfield(const char* s, i64) {
            printf("%s::.%d const\n", s, n);
        }

        //todo ! RegisterStaticFuncOther<2> for sorters
        //RegisterConst, RegisterConstBitfield, RegisterBitfield
        template <typename T, typename TInfo>
        void RegisterStruct(const char* name) {
            T* p = nullptr;
            if (T::RecordSig == Recordsig && !Found) {
                printf("%s\n", Rempref(name));
                TInfo::RegisterStructMembers(p, *this);
                if (TExtInfoType<T>::Exists)
                    WritePbInfo(TExtInfoType<T>::TResult::descriptor());
                Found = true;
            }
        }
    };
    static int DispatchInfo(ui32 recordsig) {
        TDumpRecord r(recordsig);
        TStructDescr::RegisterStructs(r);
        if (r.Found)
            return 0;
        return MBDB_BAD_RECORDSIG;
    }
    struct TSigByName {
        const TString& TypeName;
        ui32 RecordSig;
        TSigByName(const TString& n)
            : TypeName(n)
            , RecordSig(0)
        {
        }
        template <typename T, typename TInfo>
        void RegisterStruct(const char* name) {
            if (TypeName == Rempref(name))
                RecordSig = T::RecordSig;
        }
    };

    static ui32 SigByName(const TString& typeName) {
        TSigByName sb(typeName);
        TStructDescr::RegisterStructs(sb);
        return sb.RecordSig;
    }

    template <typename TRec>
    struct TLoadFinalWorker : TNullCodeProc {
        const char* From;
        TRec* To;
        int RetCode;
        int PRetCode;
        TLoadFinalWorker(TRec* t, const char* f)
            : From(f)
            , To(t)
            , RetCode(0)
            , PRetCode(777999)
        {
        }

        template <typename TAcc, typename T>
        void RegisterOrdinal(const char*, T& f) {
            RetCode = RetCode || ReadField(f, From);
            PRetCode = RetCode;
            RetCode = RetCode || *From++ != '\t';
        }
        template <int n, typename TAcc>
        void RegisterBitfield(const char*, i64) {
            i64 v = 0;
            RetCode = RetCode || ReadField(v, From);
            PRetCode = RetCode;
            RetCode = RetCode || *From++ != '\t';
            TAcc::Set(*To, v);
        }

        template <typename T>
        static int LoadExtInfo(T* extInfo, const char* ptr) {
            return google::protobuf::TextFormat::ParseFromString(ptr, extInfo) ? 0 : 1;
        }

        int FinishLoad(typename TExtInfoType<TRec>::TResult* extinfo) {
            if (!TExtInfoType<TRec>::Exists)
                if (PRetCode == 777999)
                    return *From != '\n';
                else
                    return PRetCode || *(--From) != '\n'; //last separator has been tried to read \t not \n
            else
                return RetCode || LoadExtInfo(extinfo, From);
        }
        //TODO RegisterBitfield
    };

    template <typename T, typename TInfo>
    static inline int LoadRecord(T* rec, typename TExtInfoType<T>::TResult* extinfo, const char* ptr) {
        TLoadFinalWorker<T> l(rec, ptr);
        TInfo::RegisterStructMembers(rec, l);
        return l.FinishLoad(extinfo);
    }

    template <typename T, typename TInfo>
    static int LoadData(const TFile& ifile, const TFile& ofile, size_t pagesize) {
        const size_t recsize = sizeof(T);
        const size_t fbufsize = 4u << 20;

        T* rec;
        typename TExtInfoType<T>::TResult extInfo;
        int lineno = 0;
        TOutDatFileImpl<T> datfile;

        size_t pages = std::max((size_t)2u, fbufsize / pagesize);

        if (!(rec = (T*)malloc(recsize)))
            err(1, "can't allocate %" PRISZT " bytes", recsize);

        TIFStream finput(ifile, fbufsize);

        if (datfile.Open(ofile, pagesize, pages))
            err(1, "can't write outout");

        memset(rec, 0, recsize);

        TString str;
        while (finput.ReadLine(str)) {
            str += '\n'; //may it possible to be replaced to \t ? if so upper logic may be significally simplified
            lineno++;
            if (LoadRecord<T, TInfo>(rec, &extInfo, str.data()))
                errx(1, "format error in line %d:\n<<%s>>", lineno, str.data());
            datfile.Push(rec, TExtInfoType<T>::Exists ? &extInfo : nullptr);
        }

        if (datfile.GetError())
            errx(1, "output error %d", datfile.GetError());
        if (datfile.Close())
            err(1, "can't close output");
        free(rec);
        return 0;
    }

    struct TDispatchLoad {
        const char* TypeName;
        const TFile& Ifile;
        const TFile& Ofile;
        size_t Pagesize;
        int ErrCode;
        TDispatchLoad(const char* n, const TFile& ifile, const TFile& ofile, size_t pg)
            : TypeName(n)
            , Ifile(ifile)
            , Ofile(ofile)
            , Pagesize(pg)
            , ErrCode(MBDB_BAD_RECORDSIG)
        {
        }
        template <typename T, typename TInfo>
        void RegisterStruct(const char* name) {
            if (!stricmp(TypeName, Rempref(name))) {
                //todo Bitfields can be simply supported!
                if (TInfo::InAccessible)
                    fprintf(stderr, "warning struct %s has no default ctor or has parsing errors\n", name);
                if (TInfo::ConstBitFields != 0 || ErrCode != MBDB_BAD_RECORDSIG || TInfo::InAccessible > 1 || TInfo::Consts != 0) {
                    fprintf(stderr, "struct %s can't be loaded for some reason\n", name);
                    exit(100);
                }
                ErrCode = LoadData<T, TInfo>(Ifile, Ofile, Pagesize);
            }
        }
    };

    static int DispatchLoad(const char* rectype, const TFile& ifile, const TFile& ofile, size_t pagesize) {
        TDispatchLoad sb(rectype, ifile, ofile, pagesize);
        TStructDescr::RegisterStructs(sb);
        return sb.ErrCode;
    }

    struct TAllRecordsGetter {
        TVector<std::pair<ui32, TString>> RSig2Name;
        template <typename T, typename TInfo>
        void RegisterStruct(const char* name) {
            RSig2Name.push_back(std::make_pair((ui32)T::RecordSig, TString(Rempref(name))));
        }
    };
    static int WorkLs(int, char* []) {
        TAllRecordsGetter a;
        TStructDescr::RegisterStructs(a);
        for (int i = 0; i < a.RSig2Name.ysize(); ++i)
            printf("%s\t%x\n", a.RSig2Name[i].second.data(), a.RSig2Name[i].first);
        ::Sort(a.RSig2Name.begin(), a.RSig2Name.end());
        for (int i = 1; i < a.RSig2Name.ysize(); ++i)
            if (a.RSig2Name[i - 1].first == a.RSig2Name[i].first)
                fprintf(stderr, "error %s && %s has same RecordSig %x\n", a.RSig2Name[i - 1].second.data(), a.RSig2Name[i].second.data(), a.RSig2Name[i].first);
        return IDataworkApi::E_AGAIN;
    }

    static void RegisterLs(IDataworkApi* api) {
        api->RegSomeFunc("ls", WorkLs, "ls lists all known by this scheeme structs");
    }
};
