#include <kernel/dater/dater_simple.h>

#include <util/stream/output.h>
#include <util/string/printf.h>

int main(int argc, const char** argv) {
    if (argc < 2 || TStringBuf(argv[1]) == "--help") {
        Clog << argv[0] << " <lang>" << Endl;
        exit(0);
    }

    using namespace ND2;
    TDater d;
    ELanguage l = LanguageByName(argv[1]);

    TString line;
    TString output;
    TString tmp;
    TUtf16String buf;

    while (Cin.ReadLine(line)) {
        TStringBuf first = TStringBuf(line).NextTok('\t');
        if (!first)
            continue;

        buf.clear();
        UTF8ToWide(first, buf);
        d.Clear();
        d.Localize(l);
        d.InputText = buf;
        d.Scan();

        output.clear();
        for (TDates::const_iterator it = d.OutputDates.begin(); it != d.OutputDates.end(); ++it) {
            if (it != d.OutputDates.begin())
                output.append(',');

            tmp.clear();
            sprintf(tmp, "%02u/%02u/%04u", it->Data.Day, it->Data.Month, it->Data.Year);
            output.append(tmp);
        }
        Cout << output << '\t' << line << Endl;
    }
}
