#include <kernel/voice_text/voice_text.h>

#include <library/cpp/telfinder/telfinder.h>
#include <library/cpp/getopt/small/last_getopt.h>

#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/stream/input.h>

int main(int argc, char** argv) {
    TString phoneSchemeFileName;
    TString language;
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts.AddCharOption('p', "phone schemes")
        .Required()
        .StoreResult(&phoneSchemeFileName);
    opts.AddCharOption('l', "language")
        .DefaultValue("ru")
        .StoreResult(&language);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    TTelFinder telFinder(ReadSchemes(phoneSchemeFileName));
    TString line;
    while (Cin.ReadLine(line)) {
        Cout << line << '\t';
        TUtf16String tts;
        if (TryVoiceText(UTF8ToWide(line), telFinder, tts, language)) {
            Cout << tts;
        }
        Cout << Endl;
    }
}
