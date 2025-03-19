#include "qd_printer.h"
#include "qd_server.h"
#include "qd_source.h"

#include <kernel/querydata/idl/querydata_structs.pb.h>
#include <kernel/querydata/scheme/qd_scheme.h>

#include <library/cpp/string_utils/relaxed_escaper/relaxed_escaper.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <google/protobuf/text_format.h>

namespace NQueryData {

    static inline TString& EscapeForQD(TStringBuf in, TString& out, TStringBuf unsafe = TStringBuf()) {
        out.clear();
        NEscJ::EscapeJ<false>(in, out, TStringBuf(), unsafe);
        return out;
    }

    template <typename TFactor>
    static inline char GetTypeCode(const TFactor& f) {
        if (f.HasStringValue())
            return 's';
        if (f.HasFloatValue())
            return 'f';
        if (f.HasIntValue())
            return 'i';
        if (f.HasBinaryValue())
            return 'B';
        return 0;
    }

    template <typename TFactor>
    static void DoDumpFactor(IOutputStream& out, TString& buff, TStringBuf name, const TFactor& f) {
        out << EscapeForQD(name, buff, TStringBuf(":="));

        switch (GetTypeCode(f)) {
        case 's': {
            auto s = NEscJ::EscapeJ<false>(f.GetStringValue());
            if (s == f.GetStringValue()) {
                out << ":s=" << s;
            } else {
                out << ":b=" << s;
            }
            break;
        } case 'f':
            out << ":f=" << f.GetFloatValue();
            break;
        case 'i':
            out << ":i=" << f.GetIntValue();
            break;
        case 'B':
            out << ":B=" << Base64Encode(f.GetBinaryValue());
            break;
        default:
            break;
        }
    }

    void DumpFactor(IOutputStream& out, TString& buff, const TFactor& f) {
        DoDumpFactor(out, buff, f.GetName(), f);
    }

    void DumpFactor(IOutputStream& out, TString& buff, TStringBuf name, const TRawFactor& f) {
        DoDumpFactor(out, buff, name, f);
    }

    static void DumpQueryData(IOutputStream& out, TString& buff, const TRawQueryData& qd, const TFileDescription& d) {
        if (qd.HasJson()) {
            out << '\t' << qd.GetJson();
        } else {
            for (ui32 i = 0, sz = qd.FactorsSize(); i < sz; ++i) {
                const TRawFactor& f = qd.GetFactors(i);
                out << '\t';
                DumpFactor(out, buff, d.GetFactorsMeta(f.GetId()).GetName(), f);
            }
        }
    }

    void InitPrinter(TPrinter& printer) {
        printer.SetSingleLineMode(true);
        printer.SetUseUtf8StringEscaping(true);
        printer.SetUseShortRepeatedPrimitives(true);
    }

    void Print(TBlob in, IOutputStream& out, const TPrintOpts& opts, const TString& fname) {
        TSource source;
        source.InitFake(in, "fake", 0);

        if (!source.GetTrie()) {
            out << "no trie" << Endl;
            return;
        }

        TPrinter printer;
        InitPrinter(printer);

        TRawQueryData rqdbuff;
        TBuffer buff, sbuff;
        TQueryData qdbuff;
        TServerOpts sOpts;
        sOpts.EnableDebugInfo = true;
        TQueryDatabase db(sOpts);

        TString outbuff;

        if (opts.PrintRaw) {
            out << source.GetTrie()->Report() << Endl;
            outbuff.clear();
            printer.PrintToString(source.GetDescr(), &outbuff);
            out << outbuff << "\n" << Endl;
        } else if (opts.PrintQueryData) {
            if (source.GetDescr().CommonFactorsSize()) {
                out << "#common";
                for (ui32 i = 0, sz = source.GetDescr().CommonFactorsSize(); i < sz; ++i) {
                    const TFactor& f = source.GetDescr().GetCommonFactors(i);
                    out << '\t';
                    DumpFactor(out, outbuff, f);
                }
                out << '\n';
            } else if (source.GetDescr().GetCommonJson()) {
                out << "#common\t" << source.GetDescr().GetCommonJson() << "\n";
            }
        }

        if (opts.PrintAggregated) {
            db.AddSourceData(source.GetBlob(), fname);
        }

        TAutoPtr<TQDTrie::TIterator> it = source.GetTrie()->Iterator();

        TString k, v;
        while(it->Next()) {
            k.clear();
            v.clear();
            it->Current(k, v);

            if (opts.PrintQueryData) {
                rqdbuff.Clear();
                Y_PROTOBUF_SUPPRESS_NODISCARD rqdbuff.ParseFromArray(v.data(), v.size());
                if (rqdbuff.HasKeyRef()) {
                    out << "#keyref\t" << k << '\t' << rqdbuff.GetKeyRef();
                } else {
                    out << "#query";
                    if (rqdbuff.HasVersion())
                        out << ";tstamp=" << rqdbuff.GetVersion();
                    out << "\t" << k;
                    DumpQueryData(out, outbuff, rqdbuff, source.GetDescr());
                }
                out << '\n';
            } else {
                out << k << '\n';

                if (opts.PrintRaw) {
                    rqdbuff.Clear();
                    Y_PROTOBUF_SUPPRESS_NODISCARD rqdbuff.ParseFromArray(v.data(), v.size());
                    outbuff.clear();
                    printer.PrintToString(rqdbuff, &outbuff);
                    out << outbuff << '\n';
                }

                if (opts.PrintAggregated) {
                    qdbuff.Clear();
                    db.GetQueryData(k, qdbuff);
                    NSc::TValue vv;
                    QueryData2Scheme(vv, qdbuff);
                    outbuff.clear();
                    printer.PrintToString(qdbuff, &outbuff);
                    out << outbuff << '\n';
                    vv.ToJson(out, true);
                    out << '\n';
                }

                if (opts.PrintAggregated || opts.PrintRaw)
                    out << '\n';
            }
        }

        out.Flush();
    }


    TSource::TPtr GetSource(TBlob in) {
        TSource::TPtr source = new TSource;
        source->InitFake(in, "fake", 0);
        Y_ENSURE(source->GetTrie(), "no trie");
        return source;
    }

    TSource::TPtr IterSourceFactors(TBlob in, TOnSourceFactors onSf) {
        auto source = GetSource(in);
        TQueryDatabase db;
        db.AddSource(source);

        TQueryData qd;

        if (source->GetDescr().CommonFactorsSize() || source->GetDescr().GetCommonJson()) {
            db.GetQueryData("", qd);
            for (const auto& sf : qd.GetSourceFactors()) {
                Y_ENSURE(sf.GetCommon(), "expected common");
                onSf(sf);
            }
        }

        auto it = source->GetTrie()->Iterator();

        TString k, v;
        while(it->Next()) {
            k.clear();
            v.clear();
            it->Current(k, v);
            qd.Clear();
            db.GetQueryData(k, qd);
            for (const auto& sf : qd.GetSourceFactors()) {
                Y_ENSURE(!sf.GetCommon(), "unexpected common");
                onSf(sf);
            }
        }

        return source;
    }

}
