#include <dict/easyparser/easyparser.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/streams/factory/factory.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/charset/wide.h>


int main(int argc, const char* argv[]) {
    NLastGetopt::TOpts opts;

    opts.SetTitle("Parse and lemmatize input text using easyparser library");

    TString inputPath;
    opts.AddLongOption('i', "input", "input text; '-' means stdin")
        .RequiredArgument("STRING")
        .Optional()
        .DefaultValue("-")
        .StoreResult(&inputPath);

    TString outputPath;
    opts.AddLongOption('o', "output", "output (parsed) text; '-' means stdout")
        .RequiredArgument("STRING")
        .Optional()
        .DefaultValue("-")
        .StoreResult(&outputPath);

    bool tokens = false;
    opts.AddLongOption('t', "tokens", "print original token for each input word")
        .NoArgument()
        .StoreValue(&tokens, true);

    TVector<TString> langs;
    opts.AddLongOption('l', "lang", "accept words in this language")
        .RequiredArgument("STRING")
        .Optional()
        .AppendTo(&langs);

    opts.SetFreeArgsMin(0);
    opts.SetFreeArgsMax(0);

    NLastGetopt::TOptsParseResult optsResult(&opts, argc, argv);

    TLangMask langMask;
    for (const TString& langStr: langs) {
        ELanguage lang = LANG_MAX;
        lang = LanguageByNameStrict(langStr);
        Y_VERIFY(lang != LANG_MAX, "unknown language: %s", langStr.data());
        langMask.Set(lang);
    }
    if (langs.empty()) {
        langMask.Set(LANG_RUS);
    }

    THolder<IInputStream> input = OpenInput(inputPath);
    Y_VERIFY(input);

    THolder<IOutputStream> output = OpenOutput(outputPath);
    Y_VERIFY(output);

    TString text = input->ReadAll();
    TUtf16String wideText = UTF8ToWide(text);

    TEasyParser parser{langMask};
    TEasyParser::TParseOptions parseOpts;
    if (tokens) {
        parseOpts.SaveAllTokens = true;
        parseOpts.AcceptedTypes = {NLP_WORD, NLP_INTEGER, NLP_FLOAT, NLP_MARK};
    }
    TVector<TEasyParser::TWord> words;

    parser.ParseText(wideText, &words, parseOpts);

    for (const auto& word: words) {
        if (tokens) {
            Cout << word.Token << "\t";
        }
        Cout << word.Lemma << "\t" << word.StemGrammar.ToString(",", true) << Endl;
    }

    return 0;
}
