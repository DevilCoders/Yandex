#pragma once

#include "nullcodeproc.h"
#include "datawork.h"
#include "datawork_main.h"
#include "datawork_api.h"
#include "datawork_util.h"
#include "datawork_load.h"
#include <google/protobuf/descriptor.h>
#include <library/cpp/microbdb/header.h>

#include <library/cpp/microbdb/extinfo.h>

#include "basetypestdio.h"

template <typename TStructDescr>
struct TInfo2Dispatcher {
    static const char* Rempref(const char* s) {
        const char* t = strrchr(s, ':');
        Y_VERIFY(t != nullptr, "generation error wrong typename %s\n", s);
        return t + 1;
    }

    static inline void WritePbInfo(const google::protobuf::Descriptor* d) {
        printf("ExtInfo: (%s) %s", d->file()->name().data(), d->DebugString().data());
    }

    template <typename TRec>
    struct TFullInfoRecordDump : TNullCodeProc {
        template <int argnum, typename TAcc, typename T>
        void RegisterStaticFuncOther(const char*, T) {
        }
        template <int argnum, typename TAcc, typename T>
        void RegisterFuncOtherConst(const char*, T) {
        }
        template <typename TAcc, typename T>
        void RegisterFuncConst(const char*, void (T::*)() const) {
        }

        template <typename TAcc, typename T>
        void RegisterOrdinal(const char* s, T&) {
            printf("%s::%d : %s\n", s, 8 * (int)sizeof(T), typeid(T).name());
        }
        template <typename TAcc, typename T>
        void RegisterConst(const char* s, const T&) {
            printf("%s::%d const : %s\n", s, 8 * (int)sizeof(T), typeid(T).name());
        }
        template <int n, typename TAcc>
        void RegisterConstBitfield(const char* s, i64) {
            printf("%s::%d const : int\n", s, n);
        }
        template <int n, typename TAcc>
        void RegisterBitfield(const char* s, i64) {
            printf("%s::%d : int\n", s, n);
        }
        template <typename T>
        void RegisterBaseType(const char* name) {
            printf(": %s [%s]\n", Rempref(name), name);
        }
        template <typename T, bool Const, bool Ref>
        void RegisterConvert(const char* name) {
            printf("<->%s [%s]\n", Rempref(name), name);
        }

        template <int argnum, typename TAcc, typename TRec2>
        void RegisterStaticFuncOther(const char* s, int (*)(const TRec2*, const TRec2*)) {
            printf("%s sorter\n", s);
        }
        template <int argnum, typename TAcc, typename T1, typename T2>
        void RegisterFuncOtherConst(const char*, bool (T1::*)(const T2&) const) {
            if (TAcc::Name0 == 15393 && (std::is_same<T1, T2>::value || std::is_base_of<T2, T1>::value))
                printf("- sorter (by operator <)\n");
        }
        template <typename TAcc, typename T, typename TR>
        void RegisterFuncConst(const char* n, TR (T::*)() const) {
            printf("%s() : %s\n", n, typeid(TR).name());
        }
        template <typename TAcc, typename TR>
        void RegisterFuncConst(const char* name, TR (TRec::*f)() const) {
            RegisterFuncConst<TAcc, TRec, TR>(name, f);
        }
    };

    struct TFullInfoRecord : TNullCodeProc {
        ui32 Recordsig;
        bool Found;
        TFullInfoRecord(ui32 r)
            : Recordsig(r)
            , Found(false)
        {
        }

        template <typename T, typename TInfo>
        void RegisterStruct(const char* name) {
            T* p = nullptr;
            if (T::RecordSig == Recordsig && !Found) {
                printf("%s [%s] sizeof = %d %s\n", Rempref(name), name, (int)sizeof(T), (NMicroBDB::THasSizeOf<T>::value ? "variant" : "fixed"));
                if (TInfo::InAccessible > 1)
                    printf("has inacessible fields (due to const, private, hiding, or parsing errors)\n");
                TFullInfoRecordDump<T> dmp;
                TInfo::RegisterStructMembers(p, dmp);
                if (TExtInfoType<T>::Exists)
                    WritePbInfo(TExtInfoType<T>::TResult::descriptor());
                Found = true;
            }
        }
    };
    static int DispatchInfo(ui32 recordsig) {
        TFullInfoRecord r(recordsig);
        TStructDescr::RegisterStructs(r);
        if (r.Found)
            return 0;
        return MBDB_BAD_RECORDSIG;
    }

    static void RegisterInfo2(IDataworkApi* api) {
        api->RegSomeFunc("info2", TInfoWork<TInfo2Dispatcher>::Work, "info2   -i <input>\n");
    }
};
