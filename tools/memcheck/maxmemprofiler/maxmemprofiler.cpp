#include <util/generic/hash.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <tools/memcheck/common/statparser.h>

using NMemstatsParser::TItem;
using NMemstatsParser::ReadItem;

typedef THashMap<ui64, ui64> TSizes;

int main(int argc, const char** argv) {
    if (argc < 2 || !strcmp(argv[1], "--help")) {
        Clog << "Usage: " << argv[0] << " <da_log_fname>" << Endl;
        exit(0);
    }

    TFileInput fi(argv[1], 1u<<23);
    TSizes sizes;

    ui64 nallocs = 0;
    ui64 nfrees = 0;
    ui64 mem = 0;
    ui64 maxmem = 0;

    TItem item;
    while (ReadItem(item, fi)) {
        if (item.Delete) {
            TSizes::iterator it = sizes.find(item.Ptr);
            if (it != sizes.end()) {
                mem -= it->second;
                ++nfrees;
            }
        } else {
            sizes[item.Ptr] = item.Len;
            mem += item.Len;
            maxmem = Max(mem, maxmem);
            ++nallocs;
        }
    }

    Cout << "nAllocs: " << nallocs << Endl;
    Cout << "nFrees: " << nfrees << Endl;
    Cout << "maxMem: " << maxmem << Endl;
}
