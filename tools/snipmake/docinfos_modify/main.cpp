#include <kernel/snippets/idl/snippets.pb.h>
#include <kernel/snippets/iface/archive/manip.h>
#include <kernel/tarc/iface/arcface.h>
#include <kernel/tarc/iface/tarcio.h>

#include <library/cpp/getopt/last_getopt.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <util/memory/blob.h>
#include <util/stream/zlib.h>
#include <util/stream/file.h>
#include <util/generic/hash_set.h>

using namespace NLastGetopt;
using TStrPair = std::pair<TString, TString>;
using TUrl2Attrs = THashMap<TString, TStrPair>;

bool Patch(const TString& snipCtx, const TUrl2Attrs& kv) {
    NSnippets::NProto::TSnippetsCtx ctx;
    const TString buf = Base64Decode(snipCtx);
    if (!ctx.ParseFromArray(buf.data(), buf.size())) {
        Cerr << "Bad context" << Endl;
        return false;
    }

    NSnippets::TVoidFetcher fetchText;
    NSnippets::TVoidFetcher fetchLink;
    NSnippets::TArcManip arcCtx(fetchText, fetchLink);
    if (ctx.HasTextArc())
        arcCtx.GetTextArc().LoadState(ctx.GetTextArc());

    NSnippets::TArc& tArc = arcCtx.GetTextArc();
    TDocDescr docDescr = tArc.GetDescr();
    TUrl2Attrs::const_iterator i = kv.find(docDescr.get_url());
    if (i == kv.end())
        return false;
    const TStrPair& attr = i->second;
    TDocInfos docInfos;
    docDescr.ConfigureDocInfos(docInfos);

    TDocInfoExtWriter newInfo;
    for (const auto& info : docInfos) {
        const char* name = info.first;
        const char* value = info.second;
        if (name != attr.first)
            newInfo.Add(name, value);
    }

    newInfo.Add(attr.first.data(), attr.second.data());

    // Getting blob from docDescr
    TBuffer descBlob(DEF_ARCH_BLOB);
    TDocDescr dd;
    dd.UseBlob(&descBlob);
    dd.CopyUrlDescr(docDescr);
    dd.SetUrlAndEncoding(docDescr.get_url(), docDescr.get_encoding());
    dd.set_size(docDescr.get_size());
    dd.set_urlid(docDescr.get_urlid());
    dd.set_hostid(docDescr.get_hostid());
    dd.set_mtime(docDescr.get_mtime());
    newInfo.Write(dd); // insert all infos

    ctx.MutableTextArc()->SetExtInfo(descBlob.Data(), descBlob.Size());
    Cout << Base64Encode(ctx.SerializeAsString()) << Endl;
    return true;
}

int main(int argc, char** argv) {
    TOpts opts = TOpts::Default();
    TString ctxFile;
    TString patchFile;
    bool printAll = false;
    opts.AddCharOption('i', "snippets ctx file (empty - stdin)").StoreResult(&ctxFile);
    opts.AddCharOption('p', "fastcontent document attributes file .gz\n(format described in https://wiki.yandex-team.ru/Robot/AddNewSources#3.4dannyeposnippetam)").Required().StoreResult(&patchFile);
    opts.AddCharOption('a', "print all contexts (do not skip unpatched) [def: false]").NoArgument().SetFlag(&printAll);
    TOptsParseResult(&opts, argc, argv);

    TUrl2Attrs kv;
    TUnbufferedFileInput kvFile(patchFile);
    TBufferedZLibDecompress kvReader(&kvFile);

    TString line;
    while (kvReader.ReadLine(line)) {
        TStringBuf l = line;
        TStringBuf url = l.NextTok('\t');
        TStringBuf key = l.NextTok('=');
        TStringBuf val = l;
        if (!url.empty() && !key.empty() && !val.empty())
            kv.insert(TUrl2Attrs::value_type(TString{url}, TStrPair(TString{key}, TString{val})));
    }

    THolder<TBufferedInput> ctxIn(ctxFile ? new TFileInput(ctxFile) : new TBufferedInput(&Cin));
    while (ctxIn->ReadLine(line)) {
        if (!Patch(line, kv) && printAll)
            Cout << line << Endl;
    }
    return 0;
}

