#include <library/cpp/getopt/opt.h>

#include <library/cpp/deprecated/mbitmap/mbitmap.h>

#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv)
{
    const char * bitmapPath = nullptr;
    bool help = false;

    int optlet;
    class Opt opt (argc, argv, "h?");
    while ((optlet = opt.Get()) != EOF)
    {
        switch (optlet) {
        case '?':
        case 'h':
        default:
            help = true;
            break;
        }
    }
    if (argc - opt.Ind != 1)
        help = true;
    if (help)
    {
        printf("Usage: bmdump <bitmap-file>\n");
        return 100;
    }
    bitmapPath = argv[opt.Ind];
    bitmap_1 bm;
    bm.load(bitmapPath);
    bitmap_1_iterator bmit(bm);
    while (!bmit.eof())
    {
        printf("%zu\n", bmit.doc);
        ++bmit;
    }
    bm.clear();
    return 0;
}
