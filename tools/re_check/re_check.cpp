#include <library/cpp/regex/pcre/regexp.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/generic/yexception.h>

int main(int argc, const char** argv)
{
    if (argc != 2) {
        Cout << "Usage: re_check regexp\n";
        return -1;
    }
    Cout << "RegExp: " << argv[1] << Endl;

    TRegExMatch re(argv[1]);

    TString l;
    while (Cin.ReadLine(l)) {
        bool res = re.Match(l.data());
        Cout << (res ? "Match" : "No match") << Endl;
    }
    return 0;
}

