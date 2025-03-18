#include <dict/corpus/corpus.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/system/defaults.h>

namespace {

inline void DumpText(const NCorpus::TJsonCorpus& corpus, IOutputStream& output) {
    for (size_t t = 0; t < corpus.textsSize(); ++t) {
        if (corpus.Gettexts(t).Hastext()) {
            output << corpus.Gettexts(t).Gettext() << Endl;
        }
    }
}

}

int main(int argc, char* argv[]) {
    try {
        NLastGetopt::TOpts opts;
        opts.AddLongOption('t', "type", "corpus type: 'loose', 'text', 'tags', 'readable'").RequiredArgument().DefaultValue("loose");
        opts.AddLongOption('s', "select", "text select list, comma separated ids").RequiredArgument();
        opts.AddLongOption('e', "extract", "extract only certain data (use 'type' option to set data type)").NoArgument();
        opts.AddLongOption('v', "validate", "validate (use 'type' option to set validation mode)").NoArgument();
        opts.AddLongOption('d', "dump", "dump corpus").NoArgument();
        opts.AddLongOption('o', "output", "output file path").RequiredArgument();
        opts.AddLongOption('u', "utf8", "validate UTF-8").NoArgument();
        opts.AddLongOption("gen-readable", "generate readable data").NoArgument();
        opts.AddLongOption("dump-text", "dump text data").NoArgument();
        opts.AddHelpOption();
        opts.SetFreeArgsMin(0);
        opts.SetFreeArgTitle(0, "corpus.json", "corpus file (multiple files would be merged)");
        NLastGetopt::TOptsParseResult o(&opts, argc, argv);
        TVector<TString> args(o.GetFreeArgs());

        TStringBuf type = o.Get("type");
        TStringBuf select = o.GetOrElse("select", "");
        bool extract = o.Has("extract");
        bool validate = o.Has("validate");
        bool dump = o.Has("dump");
        TString outputPath = o.GetOrElse("output", "");
        bool utf8 = o.Has("utf8");
        bool genReadable = o.Has("gen-readable");
        bool dumpText = o.Has("dump-text");

        NCorpus::ECorpusType corpusType = NCorpus::CT_LOOSE;
        if (type == "loose") {
            corpusType = NCorpus::CT_LOOSE;
        } else if (type == "text") {
            corpusType = NCorpus::CT_TEXT;
        } else if (type == "tags") {
            corpusType = NCorpus::CT_TAGS;
        } else if (type == "readable") {
            corpusType = NCorpus::CT_READABLE;
        }

        NCorpus::TJsonCorpus corpus;

        {
            IInputStream* input = &Cin;
            THolder<TIFStream> inputFileStream;

            if (!args.empty()) {
                inputFileStream.Reset(new TIFStream(args[0]));
                input = inputFileStream.Get();
            }

            corpus.Load(*input, NCorpus::CT_LOOSE, utf8, false);

            if (!args.empty()) {
                for (size_t f = 1; f < args.size(); ++f) {
                    inputFileStream.Reset(new TIFStream(args[f]));
                    NCorpus::TJsonCorpus newCorpus(*inputFileStream, NCorpus::CT_LOOSE, utf8, false);
                    corpus.Merge(newCorpus);
                }
            }
        }

        if (select) {
            THashSet<ui32> selectIds;
            TStringBuf id;
            while (select.NextTok(',', id)) {
                if (id.empty()) {
                    continue;
                }
                selectIds.insert(::FromString<ui32>(id));
            }
            corpus.Select(selectIds);
        }

        if (genReadable) {
            corpus.GenerateReadable();
        }

        if (extract) {
            corpus.Extract(corpusType);
        }

        if (validate) {
            corpus.Validate(NCorpus::TThrowingInvalidTextNotifier(), corpusType);
        }

        {
            IOutputStream* output = &Cout;
            THolder<TOFStream> outputFileStream;

            if (outputPath) {
                outputFileStream.Reset(new TOFStream(outputPath));
                output = outputFileStream.Get();
            }

            if (dump) {
                corpus.Save(*output, NCorpus::CT_LOOSE, utf8, false);
            }

            if (dumpText) {
                DumpText(corpus, *output);
            }
        }
    } catch (const yexception& error) {
        Cerr << "Error: " << error.what() << Endl;
        return 1;
    }

    return 0;
}

