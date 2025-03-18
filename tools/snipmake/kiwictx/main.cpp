#include <tools/snipmake/argv/opt.h>
#include <tools/snipmake/fatinv/fatinv.h>
#include <tools/snipmake/rawhits/rawhits.h>

#include <kernel/snippets/idl/snippets.pb.h>
#include <kernel/snippets/iface/archive/manip.h>
#include <kernel/tarc/iface/arcface.h>
#include <kernel/tarc/iface/tarcio.h>

#include <kernel/qtree/richrequest/richnode.h>

#include <yweb/robot/kiwi/clientlib/client.h>
#include <yweb/robot/kiwi/clientlib/calcclient.h>

#include <kernel/walrus/advmerger.h>

#include <library/cpp/svnversion/svnversion.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/stream/output.h>
#include <util/stream/file.h>

static bool Verbose = false;

class TWorker {
private:
    NKiwi::TSession Session;
    NKiwi::TCalcSession Session1;
    IOutputStream& Out1;
    IOutputStream& Out2;
    TString FetchReq;
    TString KiwiReq1;
    TString KiwiReq2;

private:
    static TStringBuf ReadBuf(TStringBuf& raw) {
        Y_ASSERT(raw.size() >= sizeof(ui32));
        const ui32 bufferSize = *reinterpret_cast<const ui32*>(raw.data());
        raw.Skip(sizeof(ui32));
        Y_ASSERT(raw.size() >= bufferSize);
        TStringBuf res(raw.data(), bufferSize);
        raw.Skip(bufferSize);
        return res;
    }
    static void ReadKeyInvs(TStringBuf raw, TVector<TPortionBuffers>& res) {
        while (raw.size()) {
            TStringBuf key = ReadBuf(raw);
            TStringBuf inv = ReadBuf(raw);
            res.push_back(TPortionBuffers(key.data(), key.size(), inv.data(), inv.size()));
        }
    }
    static bool ParseKiwi(const NKiwi::TKiwiObject& data, TStringBuf name, TStringBuf& val) {
        const NKiwi::NTuples::TTuple* t = data.FindByLabel(name.data(), nullptr, true);
        return t && t->GetValue(val);
    }
    static bool ParseKiwi(const NKiwi::TKiwiObject& data, TStringBuf name, TString& val) {
        TStringBuf res;
        if (ParseKiwi(data, name, res)) {
            val.assign(res.data(), res.size());
            return true;
        }
        return false;
    }

    void TransferParams(NKiwi::TCalcNamedParams& params, const NKiwi::TKiwiObject& data) {
        for (NKiwi::NTuples::TTupleIterator it = data.GetIterator(); it.IsValid(); it.Advance()) {
            const NKiwi::NTuples::TTuple* tuple = it.Current();
            if (tuple) {
                NKiwi::TExecInfoPtr info(tuple->ParseInfo());
                if (!!info) {
                    TString name = info->GetLabel();
                    if (name.size()) {
                        NKwTupleMeta::EAttrType type;
                        if (tuple->GetId() < NKwTupleMeta::ID_UDF_BASE) {
                            Session.GetAttrInfo(tuple->GetId(), name, type);
                        } else {
                            TString name1;
                            NKwTupleMeta::TFieldScheme scheme;
                            ui16 branch_ver = tuple->GetVer();
                            Session.GetUdfInfo(tuple->GetId(), branch_ver, name1, scheme);
                            type = scheme.GetType();
                        }

                        NKiwi::TCalcNamedParam param;
                        param.Name = name;
                        param.Type = type;
                        if (!tuple->IsNull()) {
                            param.Data.assign((const char*)tuple->GetData(), tuple->GetSize());
                        } else {
                            param.SetNull();
                        }
                        params.push_back(param);
                    }
                }
            }
        }
    }

    static TString GetUrl(const NSnippets::NProto::TSnippetsCtx& ctx) {
        NSnippets::TVoidFetcher fetchText;
        NSnippets::TVoidFetcher fetchLink;
        NSnippets::TArcManip arcCtx(fetchText, fetchLink);
        if (ctx.HasTextArc()) {
            arcCtx.GetTextArc().LoadState(ctx.GetTextArc());
        }
        if (ctx.HasLinkArc()) {
            arcCtx.GetLinkArc().LoadState(ctx.GetLinkArc());
        }
        TBlob DocDBlob = arcCtx.GetTextArc().GetDescrBlob(); // ref to persist
        TDocDescr DocD = arcCtx.GetTextArc().GetDescr();
        if (DocD.IsAvailable()) {
            return DocD.get_url();
        }
        return TString();
    }

    static bool DoWork(NSnippets::NProto::TSnippetsCtx ctx, const TString& url, const NKiwi::TKiwiObject& data, TString& out) {
        if (ctx.HasQuery() && ctx.GetQuery().HasQtreeBase64()) {
            TRichTreePtr richtree;
            richtree = DeserializeRichTree(DecodeRichTreeBase64(ctx.GetQuery().GetQtreeBase64()));
            if (richtree) {
                TStringBuf keyinv;
                if (!ParseKiwi(data, "keyinv", keyinv)) {
                    Cerr << "kiwi has no keyinv for " << url << Endl;
                    return false;
                }
                TVector<ui32> sents;
                TVector<ui32> masks;

                TVector<TPortionBuffers> portions;
                ReadKeyInvs(keyinv, portions);
                if (!portions.empty()) {
                    NIndexerCore::TMemoryPortion res(IYndexStorage::FINAL_FORMAT);
                    MergeMemoryPortions(&portions[0], portions.size(), IYndexStorage::FINAL_FORMAT, nullptr, false, res);
                    TString skey(res.GetKeyBuffer().Data(), res.GetKeyBuffer().Size());
                    TString sinv(res.GetInvBuffer().Data(), res.GetInvBuffer().Size());

                    AddFat(skey, sinv);

                    GetSnippetHits(sents, masks, skey, sinv, richtree);
                }
                NSnippets::NProto::THitsCtx& hct = *ctx.MutableHits();
                hct.ClearTHSents();
                hct.ClearTHMasks();
                for (size_t i = 0; i < sents.size(); ++i) {
                    hct.AddTHSents(sents[i]);
                    hct.AddTHMasks(masks[i]);
                }
                TStringBuf arc;
                TBlob extInfo;
                TBlob dataBlob;
                if (ParseKiwi(data, "arc", arc) && arc.size()) {
                    const TArchiveHeader* arcData = (const TArchiveHeader*)arc.data();
                    extInfo = GetArchiveExtInfo(arcData);
                    dataBlob = GetArchiveDocText(arcData);
                } else {
                    Cerr << "kiwi has no arc for " << url << Endl;
                    return false;
                }
                ctx.MutableTextArc()->SetData(TString(dataBlob.AsCharPtr(), dataBlob.Size()));
                ctx.MutableTextArc()->SetExtInfo(TString(extInfo.AsCharPtr(), extInfo.Size()));
            }
        }
        TBufferOutput res;
        ctx.SerializeToArcadiaStream(&res);
        out = Base64Encode(TStringBuf(res.Buffer().data(), res.Buffer().size()));
        return true;
    }

public:
    TWorker(IOutputStream& out1, IOutputStream& out2, const TString& calcHost, int calcPort, const TString& fetchReq, const TString& kiwiReq1, const TString& kiwiReq2)
      : Session(100, 100000000, 30000)
      , Session1(100, 100000000, 30000, TString(), calcHost, calcPort, 1, 1, false)
      , Out1(out1)
      , Out2(out2)
      , FetchReq(fetchReq)
      , KiwiReq1(kiwiReq1)
      , KiwiReq2(kiwiReq2)
    {
    }

    void DoWork(const NSnippets::NProto::TSnippetsCtx& ctx) {
        TString url = GetUrl(ctx);
        TString kiwiUrl = AddSchemePrefix(url);

        NKiwi::TKiwiObject inputData;
        TString inputErr;
        NKiwi::TSyncReader inputReader(Session, NKwTupleMeta::KT_DOC_DEF, FetchReq, nullptr);
        NKiwi::TReaderBase::EReadStatus inputRes = inputReader.Read(kiwiUrl, inputData, NKiwi::TReaderBase::READ_FASTEST, &inputErr);
        if (inputRes != NKiwi::TReaderBase::READ_OK && inputRes != NKiwi::TReaderBase::READ_PARTIAL) {
            Cerr << "kiwi fetch error (" << (int)inputRes << ") for " << url << ": " << inputErr << Endl;
            return;
        }

        NKiwi::TKiwiObject data1;
        {
            NKiwi::TCalcNamedParams params;
            TransferParams(params, inputData);
            TString err;
            NKiwi::TCalcSyncClient calcer(Session1);
            NKiwiCalc::EExecutionStatus calcRes = calcer.Calculate("id", KiwiReq1, params, err, data1);
            if (calcRes != NKiwiCalc::EExecutionStatus::SUCCESS && calcRes != NKiwiCalc::EExecutionStatus::PARTIAL_SUCCESS) {
                Cerr << "kiwi calc1 error (" << (int)calcRes << ") for " << url << ": " << err << Endl;
                return;
            }
        }
        NKiwi::TKiwiObject data2;
        {
            NKiwi::TCalcNamedParams params;
            TransferParams(params, inputData);
            TString err;
            NKiwi::TCalcSyncClient calcer(Session1);
            NKiwiCalc::EExecutionStatus calcRes = calcer.Calculate("id", KiwiReq2, params, err, data2);
            if (calcRes != NKiwiCalc::EExecutionStatus::SUCCESS && calcRes != NKiwiCalc::EExecutionStatus::PARTIAL_SUCCESS) {
                Cerr << "kiwi calc2 error (" << (int)calcRes << ") for " << url << ": " << err << Endl;
                return;
            }
        }
        TString ctx1;
        TString ctx2;
        if (DoWork(ctx, url, data1, ctx1) && DoWork(ctx, url, data2, ctx2)) {
            Out1 << ctx1 << Endl;
            Out2 << ctx2 << Endl;
        }
    }
};

int main(int argc, char** argv) {
    using namespace NLastGetopt;
    TOpts opt;
    TOpt& v = opt.AddLongOption("version").HasArg(NO_ARGUMENT);
    TOpt& verb = opt.AddCharOption('v', NO_ARGUMENT, "verbose output");

    TOpt& out1 = opt.AddLongOption("out1", "dump v1 ctxs to <filename>").HasArg(REQUIRED_ARGUMENT);
    TOpt& out2 = opt.AddLongOption("out2", "dump v2 ctxs to <filename>").HasArg(REQUIRED_ARGUMENT);

    TOpt& host = opt.AddLongOption("calc", "kwcalc host").HasArg(REQUIRED_ARGUMENT);
    TOpt& exactPort = opt.AddLongOption("port", "kwcalc port").HasArg(REQUIRED_ARGUMENT);
    TOpt& portShift = opt.AddLongOption("shift", "kwcalc port shift").HasArg(REQUIRED_ARGUMENT);
    TOpt& fetch = opt.AddLongOption("fetchreq", "fetch.pb.fmt").HasArg(REQUIRED_ARGUMENT);
    TOpt& req1 = opt.AddLongOption("calcreq1", "request.pb.fmt v1").HasArg(REQUIRED_ARGUMENT);
    TOpt& req2 = opt.AddLongOption("calcreq2", "request.pb.fmt v2").HasArg(REQUIRED_ARGUMENT);

    opt.SetFreeArgsNum(0);

    TOptsParseResult o(&opt, argc, argv);

    if (Has_(opt, o, &verb)) {
        Verbose = true;
    }
    if (Has_(opt, o, &v)) {
        Cout << GetProgramSvnVersion() << Endl;
        return 0;
    }
    if (!Has_(opt, o, &fetch) || !Has_(opt, o, &req1) || !Has_(opt, o, &req2)) {
        Cerr << "need all three: fetchreq, calcreq1, calcreq2 args" << Endl;
        return 1;
    }
    TString fetchReq = TUnbufferedFileInput(Get_(opt, o, &fetch)).ReadAll();
    TString kiwiReq1 = TUnbufferedFileInput(Get_(opt, o, &req1)).ReadAll();
    TString kiwiReq2 = TUnbufferedFileInput(Get_(opt, o, &req2)).ReadAll();
    TString calcHost = GetOrElse_(opt, o, &host, "kiwi.yandex.net");
    int calcPort = 31406;
    if (Has_(opt, o, &exactPort)) {
        calcPort = FromString<int>(Get_(opt, o, &exactPort));
    } else if (Has_(opt, o, &portShift)) {
        calcPort += FromString<int>(Get_(opt, o, &portShift));
    }

    TUnbufferedFileOutput fout1(GetOrElse_(opt, o, &out1, ""));
    TUnbufferedFileOutput fout2(GetOrElse_(opt, o, &out2, ""));
    TWorker wrk(fout1, fout2, calcHost, calcPort, fetchReq, kiwiReq1, kiwiReq2);

    TString s;
    while (Cin.ReadLine(s)) {
        if (!s || s.size() % 4 != 0) {
            continue;
        }
        NSnippets::NProto::TSnippetsCtx ctx;
        const TString buf = Base64Decode(s);
        if (!ctx.ParseFromArray(buf.data(), buf.size())) {
            continue;
        }
        wrk.DoWork(ctx);
    }

    return 0;
}
