#include <library/cpp/getopt/last_getopt.h>
#include <kernel/lemmer/core/morphfixlist.h>
#include <kernel/lemmer/fixlist_load/from_text.h>
#include <library/cpp/langmask/serialization/langmask.h>

#include <util/string/vector.h>

#include <kernel/reqerror/reqerror.h>
#include <ysite/yandex/reqanalysis/normalize.h>
#include <util/string/split.h>

static const TString EXCEPT("!!EXCEPTION!!");

namespace {
    class TNormalizer {
        THolder<NQueryNorm::TSimpleNormalizer> Simple_;
        THolder<NQueryNorm::TDoppNormalizer> Dopp_;
        THolder<NQueryNorm::TDoppWNormalizer> DoppW_;
        THolder<NQueryNorm::TSynNormalizer> Syn_;

    public:
        TNormalizer(const TString& gztFile, const TString& stopWordsFile, const TString& langDiscrFile)
            : Simple_(new NQueryNorm::TSimpleNormalizer)
            , Dopp_(new NQueryNorm::TDoppNormalizer)
            , DoppW_(new NQueryNorm::TDoppWNormalizer)
            , Syn_(new NQueryNorm::TSynNormalizer(gztFile, stopWordsFile, langDiscrFile))
        {
        }

        TString Simple(const TString& req, const TLangMask& /*lm*/, const TLangMask& /*prefLm*/) const {
            try {
                return (*Simple_)(req);
            } catch (const TError&) {
            }
            return EXCEPT;
        }

        TString Dopp(const TString& req, const TLangMask& /*lm*/, const TLangMask& prefLm) const {
            try {
                return (*Dopp_)(req, prefLm);
            } catch (const TError&) {
            }
            return EXCEPT;
        }

        TString DoppW(const TString& req, const TLangMask& /*lm*/, const TLangMask& /*prefLm*/) const {
            try {
                return (*DoppW_)(req);
            } catch (const TError&) {
            }
            return EXCEPT;
        }

        TString Synnorm(const TString& req, const TLangMask& lm, const TLangMask& prefLm) const {
            try {
                return (*Syn_)(req, lm, prefLm);
            } catch (const TError&) {
            }
            return EXCEPT;
        }

        bool HasSimple() const {
            return !!Simple_;
        }

        bool HasDopp() const {
            return !!Dopp_;
        }

        bool HasDoppW() const {
            return !!DoppW_;
        }

        bool HasSynnorm() const {
            return !!Syn_;
        }
    };


} //namespace

static THolder<TNormalizer> ParseArgs(int argc, char** argv) {
    using namespace NLastGetopt;
    TOpts opts;

    TOpt& optSynNormGzt = opts.AddCharOption('g', "gazetteer for synnorm").RequiredArgument("file").DefaultValue("");
    TOpt& optStopWords = opts.AddCharOption('s', "stopwords").RequiredArgument("file").DefaultValue("");
    TOpt& optMorphFixList = opts.AddCharOption('m', "fix list for morphology").RequiredArgument("file").DefaultValue("");
    TOpt& optLangFixList = opts.AddCharOption('l', "fix list for morphology language discrimination").RequiredArgument("file").DefaultValue("");

    TOptsParseResult optParseRes(&opts, argc, argv);

    if (!optParseRes.Has(&optSynNormGzt)) {
        Cerr << "-g is required" << Endl;
        opts.PrintUsage("test_norm");
        exit(1);
    }

    if (optParseRes.Has(&optMorphFixList))
        NLemmer::SetMorphFixList(optParseRes.Get(&optMorphFixList));

    return THolder(new TNormalizer(optParseRes.Get(&optSynNormGzt), optParseRes.Get(&optStopWords), optParseRes.Get(&optLangFixList)));
}

int main(int argc, char** argv) {
    THolder<TNormalizer> norm = ParseArgs(argc, argv);

    TString line;
    TVector<TString> vw;
    while (Cin.ReadLine(line)) {
        try {
            StringSplitter(line.c_str()).Split('\t').SkipEmpty().Collect(&vw);
            if (vw.size() != 3) {
                Cerr << "bad request [" << line << "]\n"
                    << "required format: [request<TAB>languages<TAB>preffered_languages]\n"
                    << "e.g. [mazda\teng,tur\teng]" << Endl;
                continue;
            }
            const TLangMask lm = DeserializeReadableLangMaskStrict(vw[1]);
            const TLangMask prefLm = DeserializeReadableLangMaskStrict(vw[2]);

            Cout << "src: " << line;
            if (norm->HasSimple())
                Cout << "\nnorm: " << norm->Simple(vw[0], lm, prefLm);
            if (norm->HasDopp())
                Cout << "\ndnorm: " << norm->Dopp(vw[0], lm, prefLm);
            if (norm->HasDoppW())
                Cout << "\ndnorm_w: " << norm->DoppW(vw[0], lm, prefLm);
            if (norm->HasSynnorm())
                Cout << "\nsynnorm: " << norm->Synnorm(vw[0], lm, prefLm);

            Cout << "\n" << Endl;
        } catch (const yexception& e) {
            Cout << e.what() << Endl;
        }
    }
}
