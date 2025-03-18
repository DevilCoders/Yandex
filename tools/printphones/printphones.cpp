#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/telfinder/text_telfinder.h>

#include <library/cpp/charset/recyr.hh>
#include <util/charset/wide.h>
#include <util/stream/file.h>

struct TPrintPhonesOptions {
    TString PathToConfig;
    bool IsUtf8;
    bool PrintStructure;
    bool MiltiStringInput;
    TVector<TString> Files;

    TPrintPhonesOptions(int argc, char *argv[]) {
        NLastGetopt::TOpts opts;

        opts.AddLongOption('c', "scheme", "path to config with schemes").RequiredArgument();
        opts.AddLongOption('u', "utf8", "input stream is in UTF-8").NoArgument();
        opts.AddLongOption('s', "struct", "print phone structure").NoArgument();
        opts.AddLongOption('m', "multistr", "assume input as one document per string").NoArgument();
        opts.AddHelpOption();

        NLastGetopt::TOptsParseResult r(&opts, argc, argv);

        if (r.Has("scheme")) {
            PathToConfig = r.Get("scheme");
        }
        IsUtf8 = r.Has("utf8");
        PrintStructure = r.Has("struct");
        MiltiStringInput = r.Has("multistr");
        Files = r.GetFreeArgs();
    };
};

void ParsePhones(const TString& text, TTelFinder* telFinder, const TPrintPhonesOptions& options) {
    //converting text to unicode
    TUtf16String wideText;
    if (options.IsUtf8) {
        wideText = UTF8ToWide(text);
    } else {
        wideText = ASCIIToWide(text);
    }

    //finding phones
    TTextTelProcessor finder(telFinder);
    TFoundPhones foundPhones;
    foundPhones.reserve(10);
    finder.ProcessText(wideText);
    finder.GetFoundPhones(foundPhones);

    TString outTelSep = "";
    //printing phones
    for (TFoundPhones::const_iterator it = foundPhones.begin(); it != foundPhones.end(); ++it) {
        const wchar16* phoneStart = finder.GetTokens()[it->Location.PhonePos.first].Word.data();
        char firstChar = it->Structure[0];
        if (firstChar == '+' || firstChar == '(') {
            phoneStart--;
        }
        const wchar16* phoneEnd = finder.GetTokens()[it->Location.PhonePos.second - 1].Word.data();

        Cout << outTelSep << it->Phone.ToPhoneWithCountry().data();
        if (options.PrintStructure) {
            Cout << " " << it->Structure;
        }
        Cout << " " << size_t(phoneStart - wideText.data());
        Cout << " " << size_t(phoneEnd - phoneStart);
        outTelSep = "\t";
    }
    Endl(Cout);
}

void ProcessStream(IInputStream& in, TTelFinder* telFinder, const TPrintPhonesOptions& options) {
    TString text;
    if (!options.MiltiStringInput) {
        text = in.ReadAll();
        ParsePhones(text, telFinder, options);
    } else {
        while (in.ReadLine(text)) {
            ParsePhones(text, telFinder, options);
        }
    }
}

int main(int argc, char *argv[]) {
    TPrintPhonesOptions  options(argc, argv);

    THolder<TTelFinder> telFinder;
    if (options.PathToConfig) {
        TPhoneSchemes schemes;
        TIFStream schemeStream(options.PathToConfig);
        schemes.ReadSchemes(schemeStream);
        telFinder.Reset(new TTelFinder(schemes));
    } else {
        telFinder.Reset(new TTelFinder);
    }

    if (options.Files.empty()) {
        ProcessStream(Cin, telFinder.Get(), options);
    } else {
        for (const TString& f: options.Files) {
            TIFStream fstr(f);
            ProcessStream(fstr, telFinder.Get(), options);
        }
    }
}
