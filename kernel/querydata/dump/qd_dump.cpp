#include "qd_dump.h"

#include <kernel/querydata/idl/querydata_structs.pb.h>
#include <kernel/querydata/client/querydata.h>

#include <library/cpp/json/json_prettifier.h>

#include <library/cpp/string_utils/relaxed_escaper/relaxed_escaper.h>

#include <util/generic/algorithm.h>
#include <util/string/printf.h>
#include <util/system/yassert.h>

namespace NQueryData {

    static void DoDumpSC(IOutputStream& out, TString reqq, const NSc::TValue& sc, bool values) {
        if (sc.IsArray()) {
            for (ui32 i = 0, sz = sc.ArraySize(); i < sz; ++i) {
                DoDumpSC(out, Sprintf("%s\t[%u]", reqq.data(), i), sc[i], values);
            }
        } else if (sc.IsDict()) {
            TStringBufs keys;
            sc.DictKeys(keys);
            Sort(keys.begin(), keys.end());
            for (TStringBufs::const_iterator it = keys.begin(); it != keys.end(); ++it) {
                DoDumpSC(out, Sprintf("%s\t%s", reqq.data(), TString{*it}.data()), sc[*it], values);
            }
        } else if (sc.IsString()) {
            out << reqq << '\t' << (values ? NEscJ::EscapeJ<false>(sc.GetString()) : ToString<int>(!!sc.GetString())) << '\n';
        } else if (sc.IsNumber()) {
            out << reqq << '\t' << (values ? sc.GetNumber() : !!sc.GetNumber()) << '\n';
        } else {
            out << reqq << '\t' << "null\n";
        }
    }

    void DumpSC(IOutputStream& out, TStringBuf req, const NSc::TValue& sc, bool values) {
        DoDumpSC(out, Sprintf("%s\tSC", TString{req}.data()), sc, values);
    }

    template <typename TFact>
    static bool DoGetFactor(TFact& fact, const TQueryDataWrapper& wr, TStringBuf src, const TKey* key, TStringBuf name) {
        return key ? wr.GetFactor(src, *key, name, fact) : wr.GetCommonFactor(src, name, fact);
    }

    static void DumpFactor(IOutputStream& out, const TQueryDataWrapper& wr, TStringBuf src, const TKey* key, TStringBuf name, bool values) {
        EFactorType ft = key ? wr.GetFactorType(src, *key, name) : wr.GetCommonFactorType(src, name);

        switch (ft) {
        case FT_STRING: {
            out << "str:";
            TString sfact;
            if (!DoGetFactor(sfact, wr, src, key, name))
                out << "??\n";
            else
                out << (values ? NEscJ::EscapeJ<false>(sfact) : ToString<int>(!!sfact)) << '\n';
            break;
        } case FT_FLOAT: {
            out << "flt:";
            float ffact = 0;
            if (!DoGetFactor(ffact, wr, src, key, name))
                out << "??\n";
            else
                out << (values ? ffact : !!ffact) << '\n';
            break;
        } case FT_INT: {
            out << "int:";
            i64 ifact = 0;
            if (!DoGetFactor(ifact, wr, src, key, name))
                out << "??\n";
            else
                out << (values ? ifact : !!ifact) << '\n';
            break;
        } case FT_NONE: {
            out << "none:\n";
            break;
        } default: {
            Y_FAIL("unknown factor type");
        }
        }
    }

    void DumpQDRaw(IOutputStream& out, TStringBuf req, const TQueryData& qd, bool values) {
        DoDumpSC(out, Sprintf("%s\tRAW", TString{req}.data()), NSc::TValue::From(qd), values);
    }

    void DumpQD(IOutputStream& out, TStringBuf req, const TQueryData& qd, bool values) {
        TQueryDataWrapper wr;
        wr.Init(qd);

        TStringBufs srcnm;
        TKeys keys;
        TStringBufs facts;

        wr.GetSourceNames(srcnm);
        Sort(srcnm.begin(), srcnm.end());

        TStringBuf json;
        for (TStringBufs::const_iterator sit = srcnm.begin(); sit != srcnm.end(); ++sit) {
            facts.clear();
            wr.GetCommonFactorNames(*sit, facts);
            Sort(facts.begin(), facts.end());

            for (TStringBufs::const_iterator fit = facts.begin(); fit != facts.end(); ++fit) {
                out << req << "\tQD\t" << *sit << "\tCOMMON_FAC\t" << *fit << '\t';
                DumpFactor(out, wr, *sit, nullptr, *fit, values);
            }

            if (wr.GetCommonJson(*sit, json)) {
                out << req << "\tQD\t" << *sit << "\tCOMMON_JSON\t" << NJson::CompactifyJson(NSc::TValue::FromJson(json).ToJson(true), true, true) << '\n';
            }

            keys.clear();
            wr.GetSourceKeys(*sit, keys);
            Sort(keys.begin(), keys.end(), TLess<const TKey*>());

            for (TKeys::const_iterator kit = keys.begin(); kit != keys.end(); ++kit) {
                out << req << "\tQD\t" << *sit << "\t" << (**kit).Dump() << '\n';

                facts.clear();
                wr.GetFactorNames(*sit, **kit, facts);

                Sort(facts.begin(), facts.end());

                for (TStringBufs::const_iterator fit = facts.begin(); fit != facts.end(); ++fit) {
                    out << req << "\tQD_FAC\t" << *sit << '\t' << (**kit).Dump() << '\t'
                                    << *fit << '\t';

                    DumpFactor(out, wr, *sit, *kit, *fit, values);
                }

                if (wr.GetJson(*sit, **kit, json)) {
                    out << req << "\tQD_JSON\t" << *sit << '\t' << (**kit).Dump() << '\t'
                                    << NJson::CompactifyJson(NSc::TValue::FromJson(json).ToJson(true), true, true) << '\n';
                }
            }
        }
    }

}
