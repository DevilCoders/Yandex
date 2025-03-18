#include <library/cpp/deprecated/dater_old/scanner/scanner.h>

#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/strip.h>

int main(int, const char**) {
    using namespace NDater;
    TString line;
    while (Cin.ReadLine(line)) {
        if (StripInPlace(line).empty())
            continue;

        const TDateCoords& c = ScanUrl(line.data(), line.size());
        Cout << line;

        for (TDateCoords::const_iterator it = c.begin(); it != c.end(); ++it) {
            if (it->YearOnly())
                continue;
            Cout << '\t' << it->ToString();
        }

        Cout << Endl;
    }
}
