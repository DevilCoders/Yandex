#include <kernel/qtree/request/req_node.h>
#include <kernel/reqerror/reqerror.h>
#include <kernel/qtree/request/request.h>
#include <kernel/qtree/richrequest/richnode.h>

#include <dict/disamb/query_disamb//langmodel.h>

#include <library/cpp/charset/wide.h>
#include <library/cpp/getopt/last_getopt.h>
#include <kernel/lemmer/core/language.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/string/cast.h>

#include <string>

struct TConfigOptions {
    TString ModelFileName;
    bool Silent;
    bool Timed;
    bool Disambiguate;
    float AbsoluteScoreLimit;
    float RelativeScoreLimit;
    bool ModelIsSerialized;
    bool SerializeModel;

    TConfigOptions()
        : Silent (false)
        , Timed (false)
        , Disambiguate (false)
        , AbsoluteScoreLimit(0)
        , RelativeScoreLimit(0)
        , ModelIsSerialized(false)
        , SerializeModel(false) {}
};

int ProcessFile(IInputStream& in, IOutputStream& out, const TConfigOptions& config) {
    TUnbufferedFileInput infile(config.ModelFileName);

    TLanguageModelManager modelmanager;
    if (config.ModelIsSerialized) {
        modelmanager.CompileSerialized(infile);
    } else {
        modelmanager.Compile(infile);
    }

    if (!config.Silent)
        Cerr << "Model loaded" << Endl;

    if (config.SerializeModel) {
        modelmanager.SaveSerialized(out);
        return 0;
    }

    struct timeval start;
    gettimeofday(&start, nullptr);

    TString line;
    size_t linecount = 0;

    while (in.ReadLine(line)) {
        linecount++;

        if (!config.Silent) {
            if (linecount % 200000 == 0)
                Cerr << linecount << Endl;
            else if (linecount % 4000 == 0)
                Cerr << ".";
        }

        TVector<TString> words;
        TVector<TString> spaces;
        TVector<TWLemmaArray> lemmagrid;
        TVector<TVector<TModelLemma> > lemmagrid_;

        // Tokenize the word, then do morphology.
        try {
            CollectWords(line.c_str(), words, spaces);
        } catch (const TError& err) {
            out << "ERROR: " << err.what() << "\t" << line << Endl;
            continue;
        }

        lemmagrid.resize(words.size());
        lemmagrid_.resize(words.size());
        const TLanguageModel& model = modelmanager.GetModel(words.size());

        TLMAnalysis bestpath(model);
        for (size_t i = 0; i < words.size(); ++i) {
            lemmagrid[i].clear();
            TUtf16String w = CharToWide(words[i], csYandex);
            NLemmer::AnalyzeWord(w.c_str(), w.length(), lemmagrid[i], LANG_RUS);
            for (size_t k = 0; k < lemmagrid[i].size(); ++k) {
                lemmagrid_[i].push_back(TModelLemma(&model, &lemmagrid[i][k], 1.0f));
            }
            bestpath.AddWord(lemmagrid_[i]);
        }
        bestpath.Finish();
        float score = bestpath.GetScore();

        if (!config.Silent) {
            if (config.Disambiguate) {
                float maxscore = score;
                if (config.RelativeScoreLimit)
                    maxscore *= (1 + config.RelativeScoreLimit);
                else if (config.AbsoluteScoreLimit)
                    maxscore += config.AbsoluteScoreLimit;

                TLMAnalysis::TSolutionEnumerator solution(bestpath, maxscore);
                out << words.size() << " " << line << Endl;
                while (solution.Next()) {
                    out << solution.GetScore();
                    TLMAnalysis::TSolutionEnumerator::TCellIterator it;
                    for (it = solution.begin(); it != solution.end(); it++) {
                        out << " ";
                        size_t lemmacount = 0;
                        for (size_t i = 0; i < 64; i++) {
                            ui64 mask = ((ui64)1) << i;
                            if ((*it)->LemmaMask & mask) {
                                if (lemmacount)
                                    out << "/";
                                lemmacount++;
                                //out << (*it)->Lemmas[i]Lemma;
                            }
                        }
                        out << "[" << model.Tagger.GetTagName((*it)->Tag) << "]";
                    }
                    out << Endl;
                }
                out << Endl;
            } else {
                out << words.size() << "\t" << score << "\t" << line << Endl;
            }
        }
    }
    if (!config.Silent)
        Cerr << linecount << Endl;

    if (config.Timed) {
        struct timeval end;
        gettimeofday(&end, nullptr);
        long millisecs = 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000;
        Cerr << linecount << " lines took " << millisecs << "  ms to process" << Endl;
    }
    return 0;
}

int SelectOutput(IInputStream& in, const TString& outfile, const TConfigOptions& config) {
    if (outfile.empty())
        return ProcessFile(in, Cout, config);

    TFixedBufferFileOutput out(outfile);

    return ProcessFile(in, out, config);
}

int SelectInput(const TString& infile, const TString& outfile, const TConfigOptions& config) {
    if (infile.empty())
        return SelectOutput(Cin, outfile, config);

    TFileInput in(infile);

    return SelectOutput(in, outfile, config);
}

int main(int argc, char* argv[]) {
    TString infile;
    TString outfile;
    TString limit;
    TConfigOptions config;

    NLastGetopt::TOpts opts;
    opts.AddHelpOption('?');
    opts.AddHelpOption('h');
    opts.AddVersionOption();

    opts.AddLongOption('o', "outfile", "Output file. If doesn't specified, results are send to stdout").RequiredArgument("FILE")
        .StoreResult(&outfile);
    opts.AddLongOption('l', "limit", "Score difference limit (absolute or percentage)")
        .StoreResult(&limit).RequiredArgument("FLOAT");
    opts.AddLongOption('d', "disambiguate", "Show disambiguation results")
        .StoreValue(&config.Disambiguate, true).NoArgument();
    opts.AddLongOption('s', "silent", "Silent mode - don't produce output (for performance metering)")
        .StoreValue(&config.Silent, true).NoArgument();
    opts.AddCharOption('t', "Print processing time  (for performance metering)")
        .StoreValue(&config.Timed, true).NoArgument();
    opts.AddLongOption('b', "binary", "Treat <model file> as binary one")
        .StoreValue(&config.ModelIsSerialized, true).NoArgument();
    opts.AddLongOption('c', "convert", "Converts (serialized) text <model file> to binary output file and exits")
        .StoreValue(&config.SerializeModel, true).NoArgument();

    opts.SetFreeArgsMin(1);
    opts.SetFreeArgsMax(2);

    opts.SetFreeArgTitle(0, "<model file>", "model file, use -b option to treat it as binary one");
    opts.SetFreeArgTitle(1, "<input file>", "If no input file is specified, data is read from stdin");

    NLastGetopt::TOptsParseResult optsParseRes(&opts, argc, argv);

    if (!limit.empty()) {
        if (strchr(limit.c_str(), '%')) {
            config.RelativeScoreLimit = 0.01f * FromString<float>(limit.substr(1));
        } else {
            config.AbsoluteScoreLimit = FromString<float>(limit);
        }
    }

    TVector<TString> freeArgs = optsParseRes.GetFreeArgs();
    if (freeArgs.size() >= 1) {
        config.ModelFileName = freeArgs[0];
    }
    if (freeArgs.size() >= 2) {
        infile = freeArgs[1];
    }

    return SelectInput(infile, outfile, config);
}
