#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <library/cpp/streams/lz/lz.h>

void PrintHelp()
{
    Cout << "usage: qlz [c|d]\n"
            " c - compress stdin to stdout\n"
            " d - decompress stdin to stdout\n";
}

int main(int argc, const char* argv[])
{
    if (argc != 2) {
        PrintHelp();
        return 1;
    }

    TString mode = argv[1];
    if (mode == "c") {
        TLzqCompress c(&Cout);
        TransferData(&Cin, &c);
    } else if (mode == "d") {
        TLzqDecompress d(&Cin);
        TransferData(&d, &Cout);
    } else {
        ythrow yexception() << "Bad mode specified";
    }

    return 0;
}
