#include "geobase.h"
#include "input.h"
#include "job.h"
#include "output.h"
#include "output_arc.h"
#include "output_csv.h"
#include "output_ctx.h"
#include "output_diff.h"
#include "output_dump.h"
#include "output_hr.h"
#include "output_hrsi.h"
#include "output_html.h"
#include "output_json.h"
#include "output_lossw.h"
#include "output_mr.h"
#include "output_print.h"
#include "output_serpf.h"
#include "output_sfh.h"
#include "output_utime.h"
#include "output_xml.h"
#include "pool_utils.h"
#include "range_time.h"

#include <tools/snipmake/argv/opt.h>
#include <tools/snipmake/common/fio.h>
#include <tools/snipmake/stopwordlst/stopword.h>
#include <tools/snipmake/metasnip/jobqueue/jobqueue.h>
#include <tools/snipmake/metasnip/jobqueue/mtptsr.h>

#include <kernel/snippets/dynamic_data/dynamic_data.h>

#include <kernel/dssm_applier/nn_applier/lib/layers.h>
#include <kernel/relevfml/models_archive/models_archive.h>

#include <library/cpp/lua/wrapper.h>
#include <library/cpp/svnversion/svnversion.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/stream/mem.h>
#include <util/system/filemap.h>
#include <util/system/info.h>
#include <util/system/nice.h>

namespace {

    THolder<IRankModelsFactory> CreateDynamicModelsFromArchive(const TString& archivePath, TBlob& dataStorage) {
        THolder<TRankModelsMapFactory> dynamicModels = MakeHolder<TRankModelsMapFactory>();
        dataStorage = TBlob::FromFile(archivePath);
        TModels models;
        NModelsArchive::LoadModels(dataStorage, models, archivePath.c_str());
        for (const auto& model : models) {
            dynamicModels->SetMatrixnet(model.first, model.second);
        }
        return THolder<IRankModelsFactory>(dynamicModels.Release());
    }

} // namespace

namespace NSnippets {

struct TReplyFilter {
    bool OnlyText = false;
    bool OnlyLink = false;
    bool NeedPassages = false;
    bool NeedEmptyPassages = false;
    TRangeTime TL;
    bool HideSpecSnippets = false;
    bool WantSpecSnippets = false;
    bool WantHeadline = false;
    TString Headline;
    TString WantAttr;
    bool WantFio = false;
    bool WantBroken = false;
    TString WantUIL;
private:
    bool HasLua = false;
    TLuaStateHolder Lua;

public:
    void InitLua(TStringBuf script) {
        if (!HasLua) {
            HasLua = true;
            Lua.BootStrap();
        }
        TMemoryInput mi(script.data(), script.size());
        Lua.Load(&mi, "main");
        Lua.call(0, 0);
    }

    bool CheckLua(const TJob& job) {
        Lua.push_global("f");
        lua_createtable(Lua, 0, 5);
        Lua.push_string("headline_src");
        Lua.push_string(job.Reply.GetHeadlineSrc());
        lua_settable(Lua, -3);
        Lua.push_string("title");
        Lua.push_string(WideToUTF8(job.Reply.GetTitle()));
        lua_settable(Lua, -3);
        Lua.push_string("headline");
        Lua.push_string(WideToUTF8(job.Reply.GetHeadline()));
        lua_settable(Lua, -3);
        Lua.push_string("passages_type");
        Lua.push_number(job.Reply.GetPassagesType());
        lua_settable(Lua, -3);
        Lua.push_string("passages");
        {
            lua_createtable(Lua, job.Reply.GetPassages().size(), 0);
            for (size_t i = 0; i < job.Reply.GetPassages().size(); ++i) {
                Lua.push_number(i + 1);
                Lua.push_string(WideToUTF8(job.Reply.GetPassages()[i]));
                lua_settable(Lua, -3);
            }
        }
        lua_settable(Lua, -3);
        Lua.call(1, 1);
        return Lua.pop_bool();
    }

    bool ShouldProcess(const TJob& job) {
        if (HasLua && !CheckLua(job)) {
            return false;
        }
        if (HideSpecSnippets && !job.Reply.GetHeadlineSrc().empty())
            return false;
        if (WantSpecSnippets && job.Reply.GetHeadlineSrc().empty())
            return false;
        if (WantHeadline && job.Reply.GetHeadlineSrc() != Headline)
            return false;
        if (OnlyText || OnlyLink) {
            if (job.Reply.GetPassagesType() != (OnlyText ? 0 : 1))
                return false;
            if (job.Exp.size() && job.ReplyExp.GetPassagesType() != (OnlyText ? 0 : 1))
                return false;
        }
        if (NeedPassages && job.Reply.GetPassages().empty()) {
            return false;
        }
        if (NeedEmptyPassages && !job.Reply.GetPassages().empty()) {
            return false;
        }
        if (!TL.Fits(job.UTime)) {
            return false;
        }
        if (WantAttr.size() && (!job.DocInfos.Get() || job.DocInfos->find(WantAttr.data()) == job.DocInfos->end())) {
            return false;
        }
        if (WantFio && (!job.Richtree.Get() || !HasFio(*job.Richtree->Root.Get()))) {
            return false;
        }
        if (WantBroken && !(job.Reply.GetError() || job.Reply.GetTitle() == EV_NOTITLE_WIDE || job.Reply.GetHeadline() == EV_NOHEADLINE_WIDE)) {
            return false;
        }
        if (WantUIL.size() && job.UIL != WantUIL) {
            return false;
        }
        return true;
    }
};

} //namespace NSnippets

const NNeuralNetApplier::TModel* PrepareDssm(NNeuralNetApplier::TModel& dssmApplier, const TString& filename) {
    dssmApplier.Load(TBlob::FromFile(filename));
    dssmApplier.Init();

    return &dssmApplier;
}

int main (int argc, char** argv)
{
    Cerr << "WARNING: this program is not complete in any sense" << Endl;

    bool printAltSnippets = false;

    using namespace NLastGetopt;
    TOpts opt;
    TOpt& v = opt.AddLongOption("version").HasArg(NO_ARGUMENT);

    opt.AddLongOption("alt", "print alt. snippets when available").HasArg(NO_ARGUMENT).SetFlag(&printAltSnippets);
    TOpt& i = opt.AddCharOption('i', REQUIRED_ARGUMENT, "input file");
    TOpt& I = opt.AddCharOption('I', REQUIRED_ARGUMENT, "input pool file in ANG export format");
    TOpt& hri = opt.AddLongOption("hri", "read text/human-readable contexts").HasArg(NO_ARGUMENT);
    TOpt& iExp = opt.AddLongOption("ie", "exp input file").HasArg(REQUIRED_ARGUMENT);

    TOpt& j = opt.AddCharOption('j', REQUIRED_ARGUMENT, "nthreads");
    TOpt& q = opt.AddCharOption('q', REQUIRED_ARGUMENT, "maxqueueJobs");
    TOpt& Q = opt.AddCharOption('Q', REQUIRED_ARGUMENT, "maxqueueRead");
    TOpt& e = opt.AddCharOption('e', REQUIRED_ARGUMENT, "exps");
    TOpt& E = opt.AddCharOption('E', REQUIRED_ARGUMENT, "reference exps");
    TOpt& r = opt.AddCharOption('r', REQUIRED_ARGUMENT, "output type: print (default), xml, diffstat, samestat, "
            "juststat, diffxml, diffctx, samectx, idiffctx, isamectx, mr_diffctx, mr_diffsamectx, ctx, json, "
            "diffjson, html, diffhtml, diffqurls, sfh, srctx, hrctx, hrsi, arc, null, ictx, loss_words, unique_ctx, utime, utimei,"
            "utimeh, requrl, dump, pool, update, imgdump, diffscore, csv, printa, printp, brief, patchctx, "
            "mr, mr_diffctx, mr_samectx, factors\n"
            "    mr - convert to mapreduce format: \n"
            "        input: key<TAB>subkey<TAB>context or id<TAB>context or context\n"
            "        output: key<TAB>subkey<TAB>context\n "
            "            key - query id,\n"
            "            subkey - snippet id within query\n"
            "        Ignores -j parameter and runs in 1 thread mode always.\n"
            "    mr_diffctx - find contexts with diff between exp and defaults,\n"
            "        input: key<TAB>subkey<TAB>context,\n"
            "        output: key<TAB>subkey<TAB>context,\n"
            "    mr_samectx - find contexts with no diff between exp and defaults,\n"
            "        input: key<TAB>subkey<TAB>context,\n"
            "        output: key<TAB>subkey<TAB>context,\n"
            "");
    TOpt& d = opt.AddCharOption('d', REQUIRED_ARGUMENT, "crash log file name");
    TOpt& x = opt.AddCharOption('x', REQUIRED_ARGUMENT, "xml file name prefix (for -r diffxml)");
    TOpt& p = opt.AddCharOption('p', REQUIRED_ARGUMENT, "probability of not skipping input (default=1)");
    TOpt& lf = opt.AddLongOption("lines", "take input lines number list to process from this file").HasArg(REQUIRED_ARGUMENT);

    TOpt& s = opt.AddCharOption('s', REQUIRED_ARGUMENT, "path/filename of the stopword list file (just get the stopwords.lst file from any production shard)");
    TOpt& ethosAnswerModelOpt = opt.AddLongOption("answer-model", "Ethos answer model storage").HasArg(REQUIRED_ARGUMENT);
    TOpt& hostStatsOpt = opt.AddLongOption("host-stats", "Factchecking hosts stats").HasArg(REQUIRED_ARGUMENT);

    TOpt& ruFactSnippetDssmApplierOpt = opt.AddLongOption("dssm-applier", "Path to ruFactSnippet DSSM for the fact snippet").HasArg(REQUIRED_ARGUMENT);
    TOpt& tomatoDssmApplierOpt = opt.AddLongOption("tomato-dssm-applier", "Path to tomato DSSM for the fact snippet").HasArg(REQUIRED_ARGUMENT);

    TOpt& es = opt.AddLongOption("es", "path/filename of the exp stopword list file").HasArg(REQUIRED_ARGUMENT);
    TOpt& t = opt.AddCharOption('t', REQUIRED_ARGUMENT, "report only slow/fast ones (arg is like +1s, -15ms, 500ms..1s) !rough!");
    TOpt& a = opt.AddCharOption('a', REQUIRED_ARGUMENT, "require archive docproperty (arg is like \"photorecipe\")");
    TOpt& c = opt.AddCharOption('c', REQUIRED_ARGUMENT, "context patch");

    TOpt& J = opt.AddCharOption('J', NO_ARGUMENT, "use ncpu threads");
    TOpt& T = opt.AddCharOption('T', NO_ARGUMENT, "print only text snippets/pairs (not with -m)");
    TOpt& L = opt.AddCharOption('L', NO_ARGUMENT, "print only link snippets/pairs (not with -m)");
    TOpt& P = opt.AddCharOption('P', NO_ARGUMENT, "print only with >=1 passages (not with -m)");
    TOpt& S = opt.AddCharOption('S', NO_ARGUMENT, "don't print spec snippets (not with -m)");
    TOpt& H = opt.AddCharOption('H', NO_ARGUMENT, "print only spec snippets (not with -m)");
    TOpt& h = opt.AddCharOption('h', REQUIRED_ARGUMENT, "filter by headline_src");
    TOpt& A = opt.AddCharOption('A', NO_ARGUMENT, "dump all algo3_pairs (only with -r dump)");
    TOpt& U = opt.AddCharOption('U', NO_ARGUMENT, "dump with unused factors (only with -r dump)");
    TOpt& M = opt.AddCharOption('M', NO_ARGUMENT, "dump with manual factors (only with -r dump)");
    TOpt& f = opt.AddCharOption('f', NO_ARGUMENT, "print only fio queries");
    TOpt& z = opt.AddCharOption('z', NO_ARGUMENT, "one criterion without ang");
    TOpt& infoReq = opt.AddLongOption("info", "info (cookie) request params; -r json output only");
    TOpt& models = opt.AddLongOption("models", "snippet.models file with additional formulae (construct with tools/archiver -qrpo snippet.models somewhere/*.info)").HasArg(REQUIRED_ARGUMENT);

    TOpt& broken = opt.AddLongOption("broken", "print only failed results (>>>> no title & such)").HasArg(NO_ARGUMENT);
    TOpt& noP = opt.AddLongOption("noP", "print only with 0 passages").HasArg(NO_ARGUMENT);
    TOpt& geoa = opt.AddLongOption("geoa", "geoa.c2p file (for imgsearch snippets)").HasArg(REQUIRED_ARGUMENT);
    TOpt& wantuil = opt.AddLongOption("wantuil", "filter by uil").HasArg(REQUIRED_ARGUMENT);
    TOpt& wantLua = opt.AddLongOption("wantlua", "filter by lua function f(attrs dict); like \"function f(x) return #x.passages == 1; end;\"").HasArg(REQUIRED_ARGUMENT);
    TOpt& nice = opt.AddLongOption("nice", "call nice() on start, default is nice(19)").HasArg(REQUIRED_ARGUMENT);

    opt.SetFreeArgsNum(0);

    TOptsParseResult o(&opt, argc, argv);

    if (Has_(opt, o, &v)) {
        Cout << GetProgramSvnVersion() << Endl;
        return 0;
    }

    if (!Nice(FromString<int>(GetOrElse_(opt, o, &nice, "19")))) {
        Cerr << "WARNING: nice() failed" << Endl;
    }

    NSnippets::TReplyFilter replyFlt;

    TString inFile = GetOrElse_(opt, o, &i, "");
    TString expInFile = GetOrElse_(opt, o, &iExp, "");
    TString inPoolFile = GetOrElse_(opt, o, &I, "");
    bool sri = Has_(opt, o, &hri);
    int nThreads = Has_(opt, o, &J) ? NSystemInfo::NumberOfCpus() : FromString<int>(GetOrElse_(opt, o, &j, "0"));
    int maxQueueSlave = FromString<int>(GetOrElse_(opt, o, &q, "-1"));
    int maxQueueSelf = FromString<int>(GetOrElse_(opt, o, &Q, "-1"));
    TString baseExp = GetOrElse_(opt, o, &E, "");
    TString exp = GetOrElse_(opt, o, &e, "");
    TString xmlPrefix = GetOrElse_(opt, o, &x, "./");
    TString outputMode = GetOrElse_(opt, o, &r, "print");
    TString log = GetOrElse_(opt, o, &d, "");
    replyFlt.OnlyText = Has_(opt, o, &T);
    replyFlt.OnlyLink = Has_(opt, o, &L);
    replyFlt.NeedPassages = Has_(opt, o, &P);
    replyFlt.NeedEmptyPassages = Has_(opt, o, &noP);
    replyFlt.HideSpecSnippets = Has_(opt, o, &S);
    replyFlt.WantSpecSnippets = Has_(opt, o, &H);
    replyFlt.WantHeadline = Has_(opt, o, &h);
    if (replyFlt.WantHeadline) {
        replyFlt.Headline = GetOrElse_(opt, o, &h, "");
    }
    replyFlt.WantBroken = Has_(opt, o, &broken);
    bool a3p = Has_(opt, o, &A);
    bool wu  = Has_(opt, o, &U);
    bool wm = Has_(opt, o, &M);
    float p_ = FromString<float>(GetOrElse_(opt, o, &p, "-1"));
    TString linesFile = GetOrElse_(opt, o, &lf, "");
    bool withoutAng = Has_(opt, o, &z);
    if (Has_(opt, o, &t)) {
        replyFlt.TL.Parse(Get_(opt, o, &t));
    }
    NSnippets::TInputFilter inputFlt(p_, linesFile);
    TString stopWordsFile = GetOrElse_(opt, o, &s, "");
    TString answerModelsFile = GetOrElse_(opt, o, &ethosAnswerModelOpt, "");
    TString hostStatsFile = GetOrElse_(opt, o, &hostStatsOpt, "");
    TString ruFactSnippetDssmApplierFile = GetOrElse_(opt, o, &ruFactSnippetDssmApplierOpt, "");
    TString tomatoDssmApplierFile = GetOrElse_(opt, o, &tomatoDssmApplierOpt, "");
    TString stopWordsExpFile = GetOrElse_(opt, o, &es, "");
    TString geoaFile = GetOrElse_(opt, o, &geoa, "");
    replyFlt.WantAttr = GetOrElse_(opt, o, &a, "");
    replyFlt.WantFio = Has_(opt, o, &f);
    replyFlt.WantUIL = GetOrElse_(opt, o, &wantuil, "");
    TString script = GetOrElse_(opt, o, &wantLua, "");
    if (script.size()) {
        replyFlt.InitLua(script);
    }

    if (replyFlt.OnlyText && replyFlt.OnlyLink) {
        Cerr << "-T and -L shouldn't be used together" << Endl;
        return 1;
    }
    if (nThreads != 0 && outputMode == "pool") {
        Cerr << "-J(or -j) and -r pool shouldn't be used together" << Endl;
        return 1;
    }
    if (outputMode == "pool" && !inPoolFile.empty()) {
        Cerr << "-I and -r pool shouldn't be used together" << Endl;
        return 1;
    }

    THolder<NSnippets::IOutputProcessor> res;
    THolder<NSnippets::IInputProcessor> inph;
    NSnippets::TPoolMaker poolMaker(outputMode == "pool");
    NSnippets::TAssessorDataManager assessorDataManager;
    if (!inPoolFile.empty()) {
        assessorDataManager.Init(inPoolFile);
    }
    NSnippets::TJobEnv jobEnv(poolMaker, assessorDataManager);

    jobEnv.InfoRequest =  GetOrElse_(opt, o, &infoReq, "");;

    TString modelsFile = GetOrElse_(opt, o, &models, "");
    TBlob modelsStorage;
    if (modelsFile.size()) {
        jobEnv.DynamicModels = CreateDynamicModelsFromArchive(modelsFile, modelsStorage);
    }

    if (nThreads != 0) {
        if (maxQueueSlave == -1) {
            maxQueueSlave = nThreads * 16;
        }
        if (maxQueueSelf == -1) {
            maxQueueSelf = maxQueueSlave * 2;
        }
    }

    if (!!inFile) {
        if (sri) {
            inph.Reset(new NSnippets::THumanFileInput(inFile));
        } else {
            inph.Reset(new NSnippets::TUnbufferedFileInput(inFile));
        }
    }
    else {
        if (sri) {
            inph.Reset(new NSnippets::THumanStdInput());
        } else if (outputMode == "patchctx") {
            TString patch = GetOrElse_(opt, o, &c, "");
            res.Reset(new NSnippets::TPatchCtxOutput(patch));
        } else {
            inph.Reset(new NSnippets::TStdInput());
        }
    }
    THolder<NSnippets::TPairedInput> pi;
    NSnippets::IInputProcessor* inp;
    if (!!expInFile) {
        pi.Reset(new NSnippets::TPairedInput(*inph.Get(), new NSnippets::TUnbufferedFileInput(expInFile)));
        inp = pi.Get();
    } else {
        inp = inph.Get();
    }
    THolder<NSnippets::IConverter> converter;
    bool skipProcess = false;

    if (outputMode == "print") {
        res.Reset(new NSnippets::TPrintOutput(Cout, false, false, false, printAltSnippets));
    } else if (outputMode == "printa") {
        res.Reset(new NSnippets::TPrintOutput(Cout, false, false, true, printAltSnippets));
    } else if (outputMode == "printp") {
        res.Reset(new NSnippets::TPrintOutput(Cout, false, true, false, printAltSnippets));
    } else if (outputMode == "xml") {
        res.Reset(new NSnippets::TXmlOutput());
    } else if (outputMode == "serpf") {
        res.Reset(new NSnippets::TSerpFormatOutput());
    } else if (outputMode == "diffstat" || outputMode == "samestat" || outputMode == "diffstata" || outputMode == "juststat") {
        res.Reset(new NSnippets::TDiffOutput(outputMode == "samestat", outputMode == "diffstata", outputMode == "juststat"));
    } else if (outputMode == "diffxml") {
        res.Reset(new NSnippets::TXmlDiffOutput(xmlPrefix));
    } else if (outputMode == "diffctx" || outputMode == "samectx") {
        res.Reset(new NSnippets::TCtxDiffOutput(outputMode == "samectx", false));
    } else if (outputMode == "idiffctx" || outputMode == "isamectx") {
        res.Reset(new NSnippets::TCtxDiffOutput(outputMode == "samectx", true));
    } else if (outputMode == "hrproto") {
        res.Reset(new NSnippets::TProtobufOutput());
    } else if (outputMode == "ctx") {
        res.Reset(new NSnippets::TCtxOutput());
    } else if (outputMode == "json") {
        res.Reset(new NSnippets::TJsonOutput());
    } else if (outputMode == "diffjson") {
        res.Reset(new NSnippets::TDiffJsonOutput());
    } else if (outputMode == "html") {
        res.Reset(new NSnippets::THtmlOutput());
    } else if (outputMode == "diffhtml") {
        res.Reset(new NSnippets::TDiffHtmlOutput());
    } else if (outputMode == "diffqurls") {
        res.Reset(new NSnippets::TDiffQurlsOutput());
    } else if (outputMode == "sfh") {
        res.Reset(new NSnippets::TSfhOutput());
    } else if (outputMode == "srctx") {
        res.Reset(new NSnippets::TSrCtxOutput());
    } else if (outputMode.StartsWith("loss_words")) {
        res.Reset(new NSnippets::TLossWordsOutput());
        baseExp.append("," + outputMode);
    } else if (outputMode == "unique_ctx") {
        res.Reset(new NSnippets::TUniqueCtxOutput());
    } else if (outputMode == "hrctx") {
        res.Reset(new NSnippets::THrCtxOutput());
    } else if (outputMode == "hrsi") {
        res.Reset(new NSnippets::THrSerpItemOutput());
    } else if (outputMode == "arc") {
        res.Reset(new NSnippets::TArcOutput());
    } else if (outputMode == "null") {
        res.Reset(new NSnippets::TNullOutput());
    } else if (outputMode == "ictx") {
        res.Reset(new NSnippets::TICtxOutput());
    } else if (outputMode == "utime" || outputMode == "utimei"){
        res.Reset(new NSnippets::TUTimeOutput(outputMode == "utimei"));
    } else if (outputMode == "utimeh") {
        res.Reset(new NSnippets::TUTimeTableOutput());
    } else if (outputMode == "requrl" || outputMode == "irequrl") {
        res.Reset(new NSnippets::TReqUrlOutput(outputMode == "irequrl"));
    } else if (outputMode == "dump" || outputMode == "pool") {
        res.Reset(new NSnippets::TDumpCandidatesOutput(inFile, outputMode == "pool", withoutAng));
        if (outputMode == "pool")
            wm = true;
        jobEnv.Adjust(a3p, wu, wm);
        baseExp = "dumpp," + baseExp;
    } else if (outputMode == "update") {
        res.Reset(new NSnippets::TUpdateCandidatesOutput(assessorDataManager));
        jobEnv.Adjust(a3p, wu, wm);
        baseExp = "dumpp," + baseExp;
        nThreads = 0;
    } else if (outputMode == "imgdump") {
        res.Reset(new NSnippets::TImgDumpOutput());
        jobEnv.Adjust(false, false, true);
        baseExp = "dumpp,imgbuild," + baseExp;
    } else if (outputMode == "diffscore") {
        res.Reset(new NSnippets::TDiffScoreOutput());
    } else if (outputMode == "csv") {
        res.Reset(new NSnippets::TCsvOutput());
    } else if (outputMode == "brief") {
        res.Reset(new NSnippets::TBriefOutput());
    } else if (outputMode == "mr") {
        converter.Reset(new NSnippets::TMRConverter());
        skipProcess = true;
    } else if (outputMode == "mr_diffctx" || outputMode == "mr_samectx") {
        res.Reset(new NSnippets::TCtxDiffOutput(outputMode == "mr_samectx", true, true));
    } else if (outputMode == "factors") {
        res.Reset(new NSnippets::TFactorsOutput());
    } else {
        Cerr << "Unsupported output mode: " << outputMode << Endl;
        return 1;
    }

    if (!!stopWordsFile) {
        jobEnv.StopWords.InitStopWordsList(stopWordsFile.data());
    } else {
        NSnippets::InitDefaultStopWordsList(jobEnv.StopWords);
    }
    THolder<NSnippets::TAnswerModels> answerModels;
    if (!!answerModelsFile) {
        answerModels.Reset(new NSnippets::TAnswerModels(answerModelsFile));
        jobEnv.AnswerModels = answerModels.Get();
    }
    THolder<NSnippets::THostStats> hostStats;
    if (!!hostStatsFile) {
        hostStats.Reset(new NSnippets::THostStats(hostStatsFile));
        jobEnv.HostStats = hostStats.Get();
    }

    NNeuralNetApplier::TModel ruFactSnippetDssmApplier;
    NNeuralNetApplier::TModel tomatoDssmApplier;
    if (!!ruFactSnippetDssmApplierFile) {
        jobEnv.RuFactSnippetDssmApplier = PrepareDssm(ruFactSnippetDssmApplier, ruFactSnippetDssmApplierFile);
    }
    if (!!tomatoDssmApplierFile) {
        jobEnv.TomatoDssmApplier = PrepareDssm(tomatoDssmApplier, tomatoDssmApplierFile);
    }

    if (!!stopWordsExpFile) {
        jobEnv.StopWordsExp.Reset(new TWordFilter);
        jobEnv.StopWordsExp->InitStopWordsList(stopWordsExpFile.data());
    }
    THolder<TImagesGeobase> geobase;
    if (!!geoaFile) {
        geobase.Reset(new TImagesGeobase(geoaFile));
        jobEnv.Geobase = geobase.Get();
    }

    NSnippets::TThreadDataManager thrData(!!log, log);
    if (skipProcess) {
        while (inp->Next()) {
            converter->Convert(inp->GetContextData());
        }
    } else {
        if (nThreads) {
            NSnippets::TMtpQueueTsr qImpl(&thrData);
            NSnippets::TJobQueue q(&qImpl);
            q.Start(nThreads, maxQueueSlave, maxQueueSelf);
            TAtomicSharedPtr<IObjectInQueue> oldJob;
            while (inp->Next()) {
                if (!inputFlt.Accept()) {
                    continue;
                }
                TAtomicSharedPtr<NSnippets::TJob> newJob(new NSnippets::TJob(inp->GetContextData(), inp->GetExpContextData(), baseExp, exp, jobEnv));
                while (!q.Add(newJob, &newJob->Account)) {
                    oldJob = q.CompleteFront();
                    if (replyFlt.ShouldProcess(*(NSnippets::TJob*)oldJob.Get())) {
                        res->Process(*(NSnippets::TJob*)oldJob.Get());
                    }
                    oldJob = nullptr;
                }
            }
            while (nullptr != (oldJob = q.CompleteFront()).Get()) {
                if (replyFlt.ShouldProcess(*(NSnippets::TJob*)oldJob.Get())) {
                    res->Process(*(NSnippets::TJob*)oldJob.Get());
                }
                oldJob = nullptr;
            }
            q.Stop();
        } else {
            void* th = thrData.CreateThreadSpecificResource();
            while (inp->Next()) {
                if (!inputFlt.Accept()) {
                    continue;
                }
                NSnippets::TJob job(inp->GetContextData(), inp->GetExpContextData(), baseExp, exp, jobEnv);
                job.Process(th);
                if (replyFlt.ShouldProcess(job)) {
                    res->Process(job);
                }
            }
            thrData.DestroyThreadSpecificResource(th);
        }
        res->Complete();
    }
    return 0;
}
