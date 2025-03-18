#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <library/cpp/streams/lz/lz.h>

void PrintHelp()
{
    Cout << "usage: ylzocat\n"
            " - decompress stdin to stdout\n";
}

int main(int argc, const char* [])
{
    if (argc != 1) {
        PrintHelp();
        return 1;
    }

    TAutoPtr<IInputStream> d(OpenLzDecompressor(&Cin));
    TransferData(d.Get(), &Cout);

    return 0;
}
