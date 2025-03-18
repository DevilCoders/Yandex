
#include <kernel/seinfo/seinfo.h>

#include <library/cpp/svnversion/svnversion.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/output.h>
#include <util/string/printf.h>

int main(int argc, char** argv)
{
    PRINT_VERSION;
    NLastGetopt::TOpts options;

    options
        .AddLongOption("fetch-max", "--  tries to fetch more")
        .NoArgument()
        .Optional();

    NLastGetopt::TOptsParseResult optParsing(&options, argc, argv);

    TString l;
    while (Cin.ReadLine(l)) {
        NSe::TInfo info = NSe::GetSeInfo(l, true, false, !optParsing.Has("fetch-max"));
        if (info.Name == NSe::SE_UNKNOWN) {
            Cout << "-\n";
        } else {
            Cout << NSe::GetSearchEngineDict()[info.Name]
                 << '\t' << ((info.Type != NSe::ST_UNKNOWN) ? NSe::GetSearchTypeDict()[info.Type] : (const char* const)"-");

            if (info.BadQuery) {
                Cout << "\tBAD\t";
            } else {
                Cout << "\t[" << info.Query << "]\t";
            }

            if (info.Flags != 0) {
                const char* separator = "";
                for (size_t bit = 1; info.Flags >= bit; bit <<= 1) {
                    if (info.Flags & bit) {
                        Cout << separator << NSe::GetSearchFlagsDict()[(NSe::ESearchFlags)bit];
                        separator = ", ";
                    }
                }
            } else {
                Cout << '-';
            }
            Cout << "\n";
        }
    }
}
