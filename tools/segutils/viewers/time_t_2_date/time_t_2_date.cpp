#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/strip.h>
#include <library/cpp/deprecated/dater_old/structs.h>

void Convert(TString s) {
    StripInPlace(s);
    Cout << (s.empty() ? TString("") : NDater::TDaterDate::FromTimeT(FromString<time_t>(s)).ToString()) << Endl;
}

int main(int argc, const char** argv) {
    if(argc > 1) {
        if(strcmp(argv[1], "--help") == 0) {
            Cerr << argv[0] << "\t<time in time_t>" << Endl;
            Cerr << "\tor" << Endl;
            Cerr << argv[0] << " < <list of time_t>" << Endl;
            return 0;
        }

        Convert(argv[1]);
        return 0;
    }

    TString line;
    while(Cin.ReadLine(line))
        Convert(line);

    return 0;
}
