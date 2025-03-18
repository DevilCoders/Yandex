#include <yweb/robot/kiwi/protos/kwworm.pb.h>

#include <yweb/robot/kiwi/clientlib/protobuf_io_bin.h>

#include <tools/snipmake/argv/opt.h>

#include <util/generic/string.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/stream/zlib.h>
#include <library/cpp/http/io/stream.h>
#include <util/network/socket.h>

bool ParseKiwi(const NKiwi::TKiwiObject& data, TStringBuf name, TStringBuf& val) {
    const NKiwi::NTuples::TTuple* t = data.FindByLabel(name.data(), nullptr, true);
    return t && t->GetValue(val);
}
bool ParseKiwi(const NKiwi::TKiwiObject& data, TStringBuf name, TString& val) {
    TStringBuf res;
    if (ParseKiwi(data, name, res)) {
        val.assign(res.data(), res.size());
        return true;
    }
    return false;
}
bool ParseKiwi(const NKiwi::TKiwiObject& data, TStringBuf name, i8& val) {
    const NKiwi::NTuples::TTuple* t = data.FindByLabel(name.data(), nullptr, true);
    return t && t->GetValue(val);
}

int main (int argc, char** argv) {
    using namespace NLastGetopt;
    TOpts opt;
    TOpt& h = opt.AddCharOption('h', REQUIRED_ARGUMENT, "host");
    TOpt& p = opt.AddCharOption('p', REQUIRED_ARGUMENT, "port");
    TOpt& m = opt.AddCharOption('m', REQUIRED_ARGUMENT, "mode");
    opt.SetFreeArgsNum(0);

    TOptsParseResult o(&opt, argc, argv);
    TString host = GetOrElse_(opt, o, &h, "");
    TString sport = GetOrElse_(opt, o, &p, "80");
    TString mode = GetOrElse_(opt, o, &m, "");

    if (!host.size()) {
        Cerr << "[-h host] is absent" << Endl;
        return 1;
    }
    if (!mode.size()) {
        Cerr << "[-m http / -m addattrs] is absent" << Endl;
        return 1;
    }

    ui16 port = FromString<ui16>(sport);

    if (mode == "http") {
        TNetworkAddress addr(host, port);
        TString s;
        while (Cin.ReadLine(s)) {
            {
                TSocket soc(addr);
                soc.SetNoDelay(true);
                {
                    TSocketOutput so(soc);
                    THttpOutput o(&so);
                    o.EnableCompression(true);
                    o.EnableKeepAlive(true);
                    TString r;
                    r += "GET /" + s + " HTTP/1.1\r\n";
                    r += "Host: " + host + ":" + port + "\r\n\r\n";
                    o.Write(r.data(), r.size());
                    o.Finish();
                }
                {
                    TSocketInput si(soc);
                    TString r = si.ReadAll();
                    TString rzip;
                    {
                        TStringOutput o(rzip);
                        TZLibCompress c(&o, ZLib::ZLib);
                        c.Write(r.data(), r.size());
                        c.Finish();
                    }
                    NKiwiWorm::TRecord rec;
                    rec.SetKey("http://" + host + "/" + s);
                    rec.SetKeytype(50);
                    NKiwiWorm::TTuple& tup = *rec.AddTuples();
                    tup.SetAttrName("HTTPResponse");
                    tup.SetBranchName("TRUNK");
                    tup.SetType(NKwTupleMeta::AT_STRING);
                    tup.SetRawData(rzip);
                    NKiwi::TProtoBinPrinter pr(Cout);
                    pr.SaveRecord(rec);
                    pr.Flush();
                }
            }
        }
    } else if (mode == "addattrs") {
        NKiwi::TSession Session(100, 100000000, 30000, TString(), host, port);
        TString kiwiReq = "html = OriginalDocUdf($HTTPResponse, false); parserChunks = ParserTrigger_robot_stable_2013_10_17(html, $URL); recognizerResult = RecognizerTrigger_robot_stable_2013_10_17(parserChunks, $URL, 0); Encoding = recognizerResult[0]; Language = recognizerResult[1]; return Encoding as Encoding, Language as Language;";
        TString s;
        while (Cin.ReadLine(s)) {
            NKiwiWorm::TRecord rec;
            rec.SetKey(s);
            rec.SetKeytype(50);

            {
                NKiwiWorm::TTuple& tup = *rec.AddTuples();
                tup.SetAttrName("MimeType");
                tup.SetBranchName("TRUNK");
                tup.SetType(NKwTupleMeta::AT_UI8);
                tup.SetStringData("2");
            }
            {
                NKiwiWorm::TTuple& tup = *rec.AddTuples();
                tup.SetAttrName("HTTPCode");
                tup.SetBranchName("TRUNK");
                tup.SetType(NKwTupleMeta::AT_UI32);
                tup.SetStringData("200");
            }
            {
                NKiwiWorm::TTuple& tup = *rec.AddTuples();
                tup.SetAttrName("Flags");
                tup.SetBranchName("TRUNK");
                tup.SetType(NKwTupleMeta::AT_UI32);
                tup.SetStringData("0");
            }
            {
                NKiwiWorm::TTuple& tup = *rec.AddTuples();
                tup.SetAttrName("TextCRC");
                tup.SetBranchName("TRUNK");
                tup.SetType(NKwTupleMeta::AT_UI64);
                tup.SetStringData("0");
            }
            {
                NKiwiWorm::TTuple& tup = *rec.AddTuples();
                tup.SetAttrName("LangRegion");
                tup.SetBranchName("TRUNK");
                tup.SetType(NKwTupleMeta::AT_UI8);
                tup.SetStringData("0");
            }
            {
                NKiwiWorm::TTuple& tup = *rec.AddTuples();
                tup.SetAttrName("LastAccess");
                tup.SetBranchName("TRUNK");
                tup.SetType(NKwTupleMeta::AT_I32);
                tup.SetStringData("0");
            }
            {
                NKiwiWorm::TTuple& tup = *rec.AddTuples();
                tup.SetAttrName("HttpModTime");
                tup.SetBranchName("TRUNK");
                tup.SetType(NKwTupleMeta::AT_I32);
                tup.SetStringData("0");
            }

            TString err;
            NKiwi::TKiwiObject data;
            NKiwi::TSyncReader reader(Session, NKwTupleMeta::KT_DOC_DEF, kiwiReq, nullptr, 100);
            NKiwi::TReaderBase::EReadStatus res = reader.Read(s, data, NKiwi::TReaderBase::READ_FASTEST, &err);
            if (res == NKiwi::TReaderBase::READ_OK || res == NKiwi::TReaderBase::READ_PARTIAL) {
                i8 lang, enc;
                TString html;
                if (ParseKiwi(data, "Language", lang)) {
                    NKiwiWorm::TTuple& tup = *rec.AddTuples();
                    tup.SetAttrName("Language");
                    tup.SetBranchName("TRUNK");
                    tup.SetType(NKwTupleMeta::AT_I8);
                    tup.SetRawData(&lang, sizeof(lang));
                }
                if (ParseKiwi(data, "Encoding", enc)) {
                    NKiwiWorm::TTuple& tup = *rec.AddTuples();
                    tup.SetAttrName("Encoding");
                    tup.SetBranchName("TRUNK");
                    tup.SetType(NKwTupleMeta::AT_I8);
                    tup.SetRawData(&enc, sizeof(enc));
                }
            } else {
                Cerr << "kiwi read error: " << err << Endl;
            }

            NKiwi::TProtoBinPrinter pr(Cout);
            pr.SaveRecord(rec);
            pr.Flush();
        }
    }
    return 0;
}
