#include <kernel/url_translit_similarity/factors.h>

#include <kernel/querydata/request/norm_query.h> // TODO: user kosher normalizer

#include <library/cpp/streams/factory/factory.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/xrange.h>
#include <util/folder/path.h>


using namespace NUrlTranslitSimilarity;

THolder<TTranslitIdfFactors> MakeCalcer(const TFsPath& path) {
    TFsPath lettersPtName = path / "letters_pt";
    TFsPath wordsPtName = path / "words_pt";
    TFsPath urlTokensFreqsName = path / "url_tokens_freqs";
    TFsPath wordFreqsName = path / "word_freqs";
    TFsPath domainIdfName = path / "domain_av_idf";

    THolder<IInputStream> lettersPtStream = OpenInput(lettersPtName);
    THolder<IInputStream> wordsPtStream = OpenInput(wordsPtName);
    THolder<IInputStream> urlTokensFreqsStream = OpenInput(urlTokensFreqsName);
    THolder<IInputStream> wordFreqsStream = OpenInput(wordFreqsName);
    THolder<IInputStream> domainIdfStream = OpenInput(domainIdfName);

    TFactorsData cfg;
    cfg.LettersPtStream = lettersPtStream.Get();
    cfg.WordsPtStream = wordsPtStream.Get();
    cfg.UrlTokensFreqsStream = urlTokensFreqsStream.Get();
    cfg.WordFreqsStream = wordFreqsStream.Get();
    cfg.DomainIdfStream = domainIdfStream.Get();

    const int searchWidth = 16;
    const double weightThreshold = 1.0;

    auto calcer = MakeHolder<TTranslitIdfFactors>();
    calcer->Load(cfg, weightThreshold, searchWidth);

    return calcer;
}

TVector<float> CalcFeatures(
    TTranslitIdfFactors& calcer,
    const TString& request,
    const TString& url)
{
    Cdbg << Endl;
    Cdbg << "UrlTranslitSimularity Begin" << Endl;
    Cdbg << "          Request ( " << request << " )" << Endl;
    TString normalizedRequest = NQueryData::SimpleNormalization(request);
    Cdbg << "       normalized ( " << normalizedRequest << " )" << Endl;

    Cdbg << "              url ( " << url << " )" << Endl;

    if (url.empty() || request.empty()) {
        Cdbg << "empty url / request" << Endl;
        TVector<float> emptyFeatures;
        emptyFeatures.assign(TFeature::Count, 0.0f);
        return emptyFeatures;
    }
    TUrlWords urlWords(url);
    Cdbg << "        host words";
    for (const auto& tWord : urlWords.HostWords) {
        Cdbg << " (" << tWord << ")";
    }
    Cdbg << Endl;

    Cdbg << "        path words";
    for (const auto& tWord : urlWords.PathWords) {
        Cdbg << " (" << tWord << ")";
    }
    Cdbg << Endl;
    TVector<float> features = calcer.ProcessStr(
        normalizedRequest,
        urlWords.HostWords,
        urlWords.PathWords);

    Cdbg << "UrlTranslitSimularity End" << Endl;
    Cdbg << Endl;

    return features;
}

int main(int argc, const char* argv[]) {
    NLastGetopt::TOpts opts;

    opts.SetTitle("Test program to compute similarity factors between search query and url texts");
    opts.AddHelpOption('h');

    TString dataDir;
    opts.AddLongOption('d', "data", "Path to directory with data files")
        .RequiredArgument("PATH")
        .Optional()
        .DefaultValue(".")
        .StoreResult(&dataDir);

    opts.SetFreeArgsNum(0);

    NLastGetopt::TOptsParseResult optionsResult(&opts, argc, argv);

    IInputStream* input = &Cin;

    THolder<TTranslitIdfFactors> calcer = MakeCalcer(dataDir);

    TString line;
    while (input->ReadLine(line)) {
        TStringBuf buf = line;
        TStringBuf request = buf.NextTok('\t');
        TStringBuf url = buf.NextTok('\t');

        TVector<float> features = CalcFeatures(*calcer, TString{request}, TString{url});

        Cout << request << '\t' << url;
        for (size_t i : xrange(features.size())) {
            Cout << '\t' << features[i];
        }
        Cout << Endl;
    }

    return 0;
}
