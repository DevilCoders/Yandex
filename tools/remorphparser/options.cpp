#include "options.h"

#include <kernel/remorph/common/verbose.h>

#include <util/stream/output.h>
#include <util/stream/file.h>
#include <util/generic/yexception.h>
#include <util/generic/ylimits.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/system/info.h>
#include <util/folder/path.h>
#include <library/cpp/charset/codepage.h>
#include <library/cpp/charset/doccodes.h>
#include <library/cpp/langs/langs.h>

namespace NRemorphParser {

static const TString INVERTERS = "~-!";

template <class TSetType, typename TFromStr>
void ParseFilterList(TString& list, TSetType& filter, TFromStr fromStr) {
    bool inverse = false;
    if (!list.empty() && INVERTERS.find(*list.begin()) != TString::npos) {
        inverse = true;
        list.remove(0, 1);
    }
    TVector<TString> v;
    Split(list, ",", v);
    for (TVector<TString>::const_iterator i = v.begin(); i != v.end(); ++i) {
        filter.SafeSet(fromStr(Strip(*i)));
    }
    if (inverse) {
        filter = ~filter;
    }
}

template <class TSetType, typename TFromStr>
void ParseFilterListOpt(const NLastGetopt::TOptsParseResult& optsResult, const TString& opt, const TString& alias, TSetType& filter, TFromStr fromStr) {
    TString value;
    if (optsResult.Has(opt)) {
        value = optsResult.Get(opt);
    } else if (optsResult.Has(alias)) {
        value = optsResult.Get(alias);
    } else {
        return;
    }
    ParseFilterList(value, filter, fromStr);
}

struct TLangFromStr {
    ELanguage operator() (const TString& str) const {
        ELanguage lang = ::LanguageByName(str);
        if (lang == LANG_UNK)
            throw NLastGetopt::TUsageException() << "Bad language value in 'xafl' param: " << str;
        return lang;
    }
};

struct TCharsetFromStr {
    ECharset operator() (const TString& str) const {
        ECharset enc = ::CharsetByName(str.data());
        if (enc == CODES_UNKNOWN)
            throw NLastGetopt::TUsageException() << "Bad encoding value in 'xafe' param: " << str;
        return enc;
    }
};

struct TMimeFromStr {
    MimeTypes operator() (const TString& str) const {
        MimeTypes mime = ::mimeByStr(str.data());
        if (mime == MIME_UNKNOWN)
            throw NLastGetopt::TUsageException() << "Bad mime-type value in 'xafm' param: " << str;
        return mime;
    }
};

TRunOpts::TRunOpts(int argc, char* argv[])
    : MatcherPath()
    , MatcherType(MT_Remorph)
    , RankMethod(NSolveAmbig::DefaultRankMethod())
    , FactMode(FM_Best)
    , MatcherMode(MM_SearchBest)
    , GztPath()
    , BaseDir()
    , AllGazetteerResults(false)
    , GazetteerRankMethod(NSolveAmbig::DefaultRankMethod())
    , InitGeoGazetteer(false)
    , Format(FMT_Text)
    , Lang()
    , Encoding(CODES_UNKNOWN)
    , InMode(IM_Plain)
    , Output(&Cout)
    , OutMode(OM_Plain)
    , PrintVerbosity(NText::PV_SHORT)
    , PrintUnmatched(false)
    , PrintInvertGrep(false)
    , Colorized(true)
    , TimeLimit(TDuration::Max())
    , Threads(1)
    , TokenizerOpts()
    , QueryPunctuation(false)
    , FilterLang()
    , FilterEncoding()
    , FilterMimeType()
    , Info(false)
    , FreeArgs()
{
    InitOptParser();
    Parse(argc, argv);
}


void TRunOpts::Parse(int argc, char* argv[]) {
    NLastGetopt::TOptsParseResult optsResult(&Options, argc, argv);
    try {
        if (optsResult.Has("info")) {
            Info = true;
            return;
        }

        if (optsResult.Has("verbose")) {
            SetVerbosityLevel(VerbosityLevelFromString(optsResult.FindLongOptParseResult("verbose")->Back()));
        }

        if (optsResult.Has("output")) {
            try {
                OutputFile.Reset(new TOFStream(optsResult.Get("output")));
            } catch (const TFileError& e) {
                throw NLastGetopt::TUsageException() << "Cannot open output file \"" << optsResult.Get("output")
                    << "\": " << e.what();
            }
            Output = OutputFile.Get();
        }

        if (optsResult.Has("format")) {
            const TString fmt = optsResult.Get("format");
            if (fmt == "text") {
                Format = FMT_Text;
            } else if (fmt == "query") {
                Format = FMT_Query;
            } else if (fmt == "index") {
                Format = FMT_Index;
            } else if (fmt == "arc") { // deprecated
                InMode = IM_Arc;
            } else if (fmt == "tarcview") { // deprecated
                InMode = IM_Tarcview;
            } else {
                throw NLastGetopt::TUsageException() << "Unsupported format: " << fmt.Quote();
            }
        }

        if (optsResult.Has("in-mode")) {
            const TString inMode = optsResult.Get("in-mode");
            if (inMode == "plain") {
                InMode = IM_Plain;
            } else if (inMode == "corpus") {
                InMode = IM_Corpus;
            } else if (inMode == "arc") {
                InMode = IM_Arc;
                if (optsResult.GetOrElse<TString>("format", "text") != "text") {
                    throw NLastGetopt::TUsageException() << "Arc input mode can only be used with text input format.";
                }
            } else if (inMode == "tarcview") {
                InMode = IM_Tarcview;
                if (optsResult.GetOrElse<TString>("format", "text") != "text") {
                    throw NLastGetopt::TUsageException() << "TarcView input mode can only be used with text input format.";
                }
            } else {
                throw NLastGetopt::TUsageException() << "Unsupported input mode: " << inMode.Quote();
            }
        }

        if ((InMode == IM_Arc) && optsResult.GetFreeArgs().empty()) {
            throw NLastGetopt::TUsageException() << "STDIN input cannot have 'arc' format";
        }

        const bool perLine = optsResult.Has("per-line");
        // Implicitly set per-line mode for tarcview
        TokenizerOpts.BlockDetection = perLine || (InMode == IM_Tarcview)
            ? NToken::BD_PER_LINE
            : NToken::BD_DEFAULT;
        if (perLine) {
            TokenizerOpts.Set(NToken::TF_NO_SENTENCE_SPLIT);
        }

        const TString split = optsResult.Get("split");
        if (split == "minimal") {
            TokenizerOpts.MultitokenSplit = NToken::MS_MINIMAL;
        } else if (split == "smart") {
            TokenizerOpts.MultitokenSplit = NToken::MS_SMART;
        } else if (split == "all") {
            TokenizerOpts.MultitokenSplit = NToken::MS_ALL;
        } else {
            throw NLastGetopt::TUsageException() << "Bad 'split' param value: " << split;
        }

        if (optsResult.Has("max-tokens")) {
            try {
                TokenizerOpts.MaxSentenceTokens = optsResult.Get<size_t>("max-tokens");
                if (0 == TokenizerOpts.MaxSentenceTokens)
                    TokenizerOpts.MaxSentenceTokens = Max();
            } catch (const yexception& e) {
                throw NLastGetopt::TUsageException() << "Bad 'max-tokens' param value: " << e.what();
            }
        }

        try {
            Lang = NLanguageMasks::CreateFromList(optsResult.Get("lang"));
        } catch (const yexception& e) {
            throw NLastGetopt::TUsageException() << "Bad 'lang' param value: " << e.what();
        }

        Encoding = ::CharsetByName(optsResult.Get("encoding"));
        if (CODES_UNSUPPORTED == Encoding) {
            throw NLastGetopt::TUsageException() << "Unsupported encoding: " << optsResult.Get("encoding");
        }

        QueryPunctuation = optsResult.Has("query-punct");

        if (optsResult.Has("threads")) {
            const NLastGetopt::TOptParseResult* res = optsResult.FindLongOptParseResult("threads");
            try {
                Threads = !res || res->Empty() ? NSystemInfo::CachedNumberOfCpus() : FromString(res->Back());
            } catch (const TFromStringException& e) {
                throw NLastGetopt::TUsageException() << "Bad 'threads' param value: " << res->Back();
            }
        }

        if (optsResult.Has("out-mode")) {
            TString mode = optsResult.Get<TString>("out-mode");
            if (mode == "short") {
                OutMode = OM_Plain;
                PrintVerbosity = NText::PV_SHORT;
            } else if (mode == "full") {
                OutMode = OM_Plain;
                PrintVerbosity = NText::PV_FULL;
            } else if (mode == "corpus") {
                OutMode = OM_Plain;
                PrintVerbosity = NText::PV_TEXTONLY;
            } else if (mode == "grep") {
                OutMode = OM_Plain;
                PrintVerbosity = NText::PV_GREP;
            } else if (mode == "igrep") {
                OutMode = OM_Plain;
                PrintVerbosity = NText::PV_GREP;
                PrintInvertGrep = true;
            } else if (mode == "corpus-tags") {
                OutMode = OM_CorpusTags;
                if (!optsResult.Has("facts")) {
                    throw NLastGetopt::TUsageException() << "Corpus tags output mode can only be used with facts.";
                }
                if (optsResult.GetOrElse<TString>("format", "text") != "text") {
                    throw NLastGetopt::TUsageException() << "Corpus tags output mode can only be used with text input format.";
                }
            } else if (mode == "facts-json") {
                OutMode = OM_FactsJson;
                if (!optsResult.Has("facts")) {
                    throw NLastGetopt::TUsageException() << "Facts JSON output mode can only be used with facts.";
                }
            } else {
                throw NLastGetopt::TUsageException() << "Bad 'out-mode' value: " << optsResult.Get<TString>("out-mode").Quote();
            }
        }

        AllGazetteerResults = optsResult.Has("all-gzt");
        PrintUnmatched = optsResult.Has("unmatched");

        if (optsResult.Has("mode")) {
            const TString mode = optsResult.Get<TString>("mode");
            if (mode == "match") {
                MatcherMode = MM_Match;
            } else if (mode == "match-all") {
                MatcherMode = MM_MatchAll;
            } else if (mode == "match-best") {
                MatcherMode = MM_MatchBest;
            } else if (mode == "search") {
                MatcherMode = MM_Search;
            } else if (mode == "search-all") {
                MatcherMode = MM_SearchAll;
            } else if (mode == "search-best") {
                MatcherMode = MM_SearchBest;
            } else if (mode == "solutions") {
                MatcherMode = MM_Solutions;
            } else {
                throw NLastGetopt::TUsageException() << "Bad 'mode' value: " << optsResult.Get<TString>("mode").Quote();
            }
        }

        if (optsResult.Has("fact-ambig")) {
            TString mode = optsResult.Get<TString>("fact-ambig");
            if (mode == "best") {
                FactMode = FM_Best;
            } else if (mode == "all") {
                FactMode = FM_All;
            } else if (mode == "solutions") {
                FactMode = FM_Solutions;
            } else {
                throw NLastGetopt::TUsageException() << "Bad 'fact-ambig' value: " << optsResult.Get<TString>("fact-ambig").Quote();
            }
        }

        size_t cnt = optsResult.Has("remorph") + optsResult.Has("tokenlogic") + optsResult.Has("char") + optsResult.Has("facts");
        if (cnt > 1) {
            throw NLastGetopt::TUsageException() << "Only one of the 'remorph', 'tokenlogic', 'char', 'facts' options can be specified";
        } else if (cnt == 0) {
            throw NLastGetopt::TUsageException() << "One of the 'remorph', 'tokenlogic', 'char', 'facts' options must be specified";
        }

        if (optsResult.Has("facts")) {
            MatcherType = MT_Fact;
            MatcherPath = optsResult.Get("facts");
        } else if (optsResult.Has("remorph")) {
            MatcherType = MT_Remorph;
            MatcherPath = optsResult.Get("remorph");
        } else if (optsResult.Has("tokenlogic")) {
            MatcherType = MT_Tokenlogic;
            MatcherPath = optsResult.Get("tokenlogic");
        } else if (optsResult.Has("char")) {
            MatcherType = MT_Char;
            MatcherPath = optsResult.Get("char");
        }
        BaseDir = TFsPath(MatcherPath).Parent().RealPath().c_str();

        if (optsResult.Has("gzt-file")) {
            GztPath = optsResult.Get("gzt-file");
            BaseDir = TFsPath(GztPath).Parent().RealPath().c_str();
        }

        ParseFilterListOpt(optsResult, "xafl", "filter-lang", FilterLang, TLangFromStr());
        ParseFilterListOpt(optsResult, "xafe", "filter-encoding", FilterEncoding, TCharsetFromStr());
        ParseFilterListOpt(optsResult, "xafm", "filter-mime", FilterMimeType, TMimeFromStr());

        if (optsResult.Has("monitor")) {
            try {
                TimeLimit = TDuration::Parse(optsResult.Get<TString>("monitor"));
            } catch (const yexception& e) {
                throw NLastGetopt::TUsageException() << "Bad 'monitor' param value: " << e.what();
            }
        }

        if (optsResult.Has("rank")) {
            RankMethod = NSolveAmbig::TRankMethod(optsResult.Get("rank"));
        }

        if (optsResult.Has("rank-gzt")) {
            GazetteerRankMethod = NSolveAmbig::TRankMethod(optsResult.Get("rank-gzt"));
        }

        if (optsResult.Has("geo-gzt")) {
            InitGeoGazetteer = true;
        }

        if (optsResult.Has("no-color")) {
            Colorized = false;
        }

        FreeArgs = optsResult.GetFreeArgs();
    } catch (const yexception&) {
        optsResult.HandleError();
        exit(1);
    }
}

void TRunOpts::InitOptParser() {
    // Processing options
    Options.AddLongOption('f', "facts", "fact descriptor path").RequiredArgument();
    Options.AddLongOption('k', "fact-ambig", "fact ambiguity resolving mode: 'best', 'all', 'solutions'").RequiredArgument().DefaultValue("best");
    Options.AddLongOption("rank", "ranking method for choosing the best results, facts or solutions while resolving, comma-separated").RequiredArgument();
    Options.AddLongOption('r', "remorph", "remorph rules path").RequiredArgument();
    Options.AddLongOption("tokenlogic", "tokenlogic rules path").RequiredArgument();
    Options.AddLongOption('c', "char", "char rules path").RequiredArgument();
    Options.AddLongOption('m', "mode", "rules processing mode: 'solutions', 'match', 'match-all', 'match-best', 'search', 'search-all', 'search-best'").RequiredArgument().DefaultValue("search-best");
    Options.AddLongOption('g', "gzt-file", "gazetteer dictionary path").RequiredArgument();
    Options.AddLongOption("all-gzt", "use all gazetteer results, don't resolve gazetteer ambiguity").NoArgument();
    Options.AddLongOption("rank-gzt", "ranking method for choosing the best gazetteer results while resolving, comma-separated").RequiredArgument();
    Options.AddLongOption("geo-gzt", "init geogazetteer for geo-agreement").NoArgument();

    // Input options.
    Options.AddLongOption('t', "format", "input text format: 'text', 'query' or 'index'").RequiredArgument().DefaultValue("text");
    Options.AddLongOption('l', "lang", "input languages, comma-separated").RequiredArgument().DefaultValue("rus");
    Options.AddLongOption('e', "encoding", "input encoding").RequiredArgument().DefaultValue("utf-8");
    Options.AddLongOption("in-mode", "input mode: 'plain', 'corpus', 'arc', 'tarcview'").RequiredArgument().DefaultValue("plain");

    // Output options.
    Options.AddLongOption('o', "output", "output path").RequiredArgument();
    Options.AddLongOption('h', "out-mode", "output mode: 'short', 'full', 'corpus', 'corpus-tags', 'facts-json', 'grep', 'igrep'").RequiredArgument().DefaultValue("short");
    Options.AddLongOption('v', "verbose", "verbose stderr logging: 'err(or)', 'warn(ing)', 'notice', 'info', 'debug', 'detail', 'verbose' or 1-7 numbers, zero for disabled logging").OptionalArgument().DefaultValue("info");
    Options.AddLongOption('u', "unmatched", "output all sentences including unmatched ones").NoArgument();
    Options.AddLongOption("monitor", "report documents being processed longer than this specified threshold").RequiredArgument();
    Options.AddLongOption("no-color", "disable colorized output").NoArgument();

    // Misc options.
    Options.AddLongOption('j', "threads", "number of threads for multithreaded mode, omit value to for auto").OptionalArgument();
    Options.AddLongOption('p', "per-line", "process each line as a separate text block").NoArgument();
    Options.AddLongOption('s', "split", "multitoken split mode: 'minimal', 'smart', 'all'").RequiredArgument().DefaultValue("minimal");
    Options.AddLongOption("query-punct", "delimiters processing for query input").NoArgument();
    Options.AddLongOption("max-tokens", "maximum number of tokens per sentence, zero for unlimited").RequiredArgument().DefaultValue("100");

    // Developer options.
    Options.AddLongOption("xafl", "[dev] filter arc or tarcview docs by languages, prefix list by '~', '!', or '-' to exclude").RequiredArgument();
    Options.AddLongOption("xafe", "[dev] filter arc or tarcview docs by encodings, prefix list by '~', '!', or '-' to exclude").RequiredArgument();
    Options.AddLongOption("xafm", "[dev] filter arc or tarcview docs by MIME types, prefix list by '~', '!', or '-' to exclude").RequiredArgument();

    // Deprecated options.
    Options.AddLongOption("filter-lang", "alias for --xafl (deprecated)").RequiredArgument().Hidden();
    Options.AddLongOption("filter-encoding", "alias for --xafe (deprecated)").RequiredArgument().Hidden();
    Options.AddLongOption("filter-mime", "alias for --xafm (deprecated)").RequiredArgument().Hidden();

    // Special options.
    Options.AddLongOption("info", "print remorph info").NoArgument();
    Options.AddHelpOption();
}

} // NRemorphParser
