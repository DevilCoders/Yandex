
#include <library/cpp/digest/old_crc/crc.h>
#include <util/generic/string.h>
#include <util/stream/input.h>
#include <util/stream/output.h>

void PrintHelp()
{
    Cout << "usage: crc32\n"
            "computes crc32 of stdin\n";
}

int main(int, const char**)
{
    TString data = Cin.ReadAll();
    Cout << crc32(data.data(), data.size()) << Endl;

    return 0;
}
