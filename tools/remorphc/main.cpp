#include "compiler.h"
#include "config.h"
#include "config_loader.h"

#include <kernel/remorph/common/verbose.h>
#include <kernel/remorph/core/core.h>
#include <kernel/remorph/info/info.h>
#include <kernel/remorph/matcher/rule_parser.h>
#include <kernel/remorph/proc_base/matcher_base.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/stream/trace.h>
#include <util/system/info.h>
#include <utility>

using namespace NRemorphCompiler;

namespace {

void DumpRemorphNfa(const TString& rulesPath, IOutputStream& output) {
    TIFStream stream(rulesPath);
    NReMorph::NPrivate::TParseResult rules;
    NReMorph::NPrivate::ParseFile(rules, rulesPath, stream);
    NRemorph::TNFAPtr nfa = NRemorph::Combine(rules.NFAs);
    NRemorph::PrintAsDot(output, *rules.LiteralTable, *nfa);
}

void DumpRemorphDfa(const TString& rulesPath, IOutputStream& output) {
    TIFStream stream(rulesPath);
    NReMorph::NPrivate::TParseResult rules;
    NReMorph::NPrivate::ParseFile(rules, rulesPath, stream);
    NRemorph::TDFAPtr dfa = NRemorph::Convert(*rules.LiteralTable, rules.NFAs);
    NRemorph::PrintAsDot(output, *rules.LiteralTable, *dfa);
}

}

int main(int argc, char* argv[]) {
    try {
        NLastGetopt::TOpts opts;

        opts.AddLongOption('t', "type", "input type: 'config', 'remorph', 'tokenlogic', 'char'").RequiredArgument().DefaultValue("config");
        opts.AddLongOption('o', "output", "output path").RequiredArgument();
        opts.AddLongOption('g', "gzt-file", "gazetteer dictionary path").RequiredArgument();
        opts.AddLongOption('i', "base", "base directory for sub-cascades search").RequiredArgument();
        opts.AddLongOption('r', "recursive", "load configuration recursively from .remorph files").NoArgument();
        opts.AddLongOption('j', "threads", "number of threads, zero value for auto").OptionalArgument().OptionalValue("0");
        opts.AddLongOption('v', "verbose", "verbose stderr logging: 'err(or)', 'warn(ing)', 'notice', 'info', 'debug', 'detail', 'verbose' or 1-7 numbers, zero for disabled logging").OptionalArgument().OptionalValue("info");
        opts.AddLongOption("vars", "build configuration variables, comma-separated, 'name1=value1,name2=value2')").OptionalArgument();

        // Developer options.
        opts.AddLongOption("xdn", "[dev] output rules NFA in dot format").NoArgument();
        opts.AddLongOption("xdd", "[dev] output rules DFA in dot format").NoArgument();
        opts.AddLongOption("xnc", "[dev] do not compile").NoArgument();

        // Special options.
        opts.AddLongOption("info", "print remorph info").NoArgument();
        opts.AddHelpOption();

        opts.SetFreeArgsMin(0);
        opts.SetFreeArgsMax(1);
        opts.SetFreeArgTitle(0, "input", "input file or directory");

        opts.SetAllowSingleDashForLong(true);
        NLastGetopt::TOptsParseResult r(&opts, argc, argv);
        TVector<TString> args(r.GetFreeArgs());

        if (r.Has("info")) {
            Cout << NReMorph::REMORPH_INFO << Endl;
            return 0;
        }

        bool optionsError = false;

        TString type = r.Get("type");
        bool recursive = r.Has("recursive");

        TString input;
        TVars vars;

        if (args.size() < 1) {
            if (recursive) {
                input = ".";
            } else if (type == "config") {
                input = GetDefaultConfigName();

                if (!TFsPath(input).Exists()) {
                    Cerr << "Compiler uses \"" << input << "\" configuration file if no input is explicitly specified." << Endl;
                    Cerr << "No \"" << input << "\" file found in the working directory." << Endl;
                    optionsError = true;
                }
            } else {
                Cerr << "No input file specified." << Endl;
                optionsError = true;
            }
        } else {
            input = args[0];
        }

        if ((type == "config") && (r.Has("output") || r.Has("gzt-file") || r.Has("base"))) {
            Cerr << "Options 'output', 'gzt-file' and 'base' cannot be specified together with --type=config or without explicit rules type specification." << Endl;
            optionsError = true;
        }

        if ((type != "config") && r.Has("recursive")) {
            Cerr << "Option 'recursive' can only be specified with --type=config (or without explicit type specification)." << Endl;
            optionsError = true;
        }

        if ((type != "remorph") && (r.Has("xdn") || r.Has("xdd"))) {
            Cerr << "Options 'xdn' and 'xdd' can only be specified together with --type=remorph." << Endl;
            optionsError = true;
        }

        if (r.Has("xdn") && r.Has("xdd")) {
            Cerr << "Options 'xdn' and 'xdd' cannot be passed together." << Endl;
            optionsError = true;
        }

        if (optionsError) {
            opts.PrintUsage(r.GetProgramName());
            return 1;
        }

        bool xDotNfa = r.Has("xdn");
        bool xDotDfa = r.Has("xdd");
        bool xNoCompile = r.Has("xnc");

        if (xDotNfa || xDotDfa) {
            IOutputStream* dotOutput = &Cout;
            THolder<TOFStream> dotOutputFile;

            if (r.Has("output")) {
                dotOutputFile.Reset(new TOFStream(r.Get("output")));
                dotOutput = dotOutputFile.Get();
            }

            if (xDotNfa) {
                DumpRemorphNfa(input, *dotOutput);
            } else if (xDotDfa) {
                DumpRemorphDfa(input, *dotOutput);
            }

            return 0;
        }

        if (r.Has("vars")) {
            TVector<TString> elems;
            StringSplitter(r.Get("vars")).Split(',').SkipEmpty().Collect(&elems);
            for (TVector<TString>::const_iterator iElem = elems.begin(); iElem != elems.end(); ++iElem) {
                const TString& elem = *iElem;
                size_t delim = elem.find('=');
                if ((delim == TString::npos) || !delim) {
                    throw yexception() << "Invalid variable specification: " << elem.Quote() << ", must be in form name=value";
                }
                TString name = elem.substr(0, delim);
                TString value = elem.substr(delim + 1);
                if (vars.find(name) != vars.end()) {
                    throw yexception() << "Variable already defined: " << name;
                }
                vars.insert(::std::make_pair(name, value));
            }
        }

        bool single = type != "config";
        IOutputStream* log = !single ? &Cerr : nullptr;
        bool throwOnError = single;

        TConfig config;

        if (!single) {
            TConfigLoader configLoader(config, vars, log);
            configLoader.Load(input, recursive);
        } else {
            NMatcher::EMatcherType rulesType;
            if (type == "remorph") {
                rulesType = NMatcher::MT_REMORPH;
            } else if (type == "tokenlogic") {
                rulesType = NMatcher::MT_TOKENLOGIC;
            } else if (type == "char") {
                rulesType = NMatcher::MT_CHAR;
            } else {
                throw yexception() << "Unsupported rules type: \"" << type << "\"";
            }

            config.GetUnits().push_back(TUnitConfig::TPtr(new TUnitConfig(input, rulesType)));
            TUnitConfig::TPtr& unitConfig = config.GetUnits().back();

            if (r.Has("output")) {
                unitConfig->SetOutput(r.Get("output"));
            }

            if (r.Has("base")) {
                unitConfig->SetGazetteerBase(r.Get("base"));
            }

            if (r.Has("gzt-file")) {
                unitConfig->SetGazetteer(r.Get("gzt-file"));
            }
        }

        if (config.Empty()) {
            throw yexception() << "Nothing to do (configuration is empty)";
        }

        size_t threads = 1u;
        if (r.Has("threads")) {
            size_t th = r.Get<size_t>("threads");
            threads = (th > 0u) ? th : NSystemInfo::CachedNumberOfCpus();
        }

        ETraceLevel verbosity = TRACE_WARN;
        if (r.Has("verbose")) {
            verbosity = static_cast<ETraceLevel>(VerbosityLevelFromString(r.Get("verbose")));
        }

        if (xNoCompile) {
            return 0;
        }

        TCompiler compiler(threads, verbosity);
        bool status = compiler.Run(config, log, throwOnError);

        return status ? 0 : 3;
    } catch (yexception& error) {
        Cerr << "ERROR: " << error.what() << Endl;
        return 2;
    }
    return 0;
}
