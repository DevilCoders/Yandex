/// author@ cheusov@ Aleksey Cheusov
/// created: Sun, 12 Oct 2014 18:10:15 +0300
/// see: OXYGEN-890

#include <kernel/xref/xref_types.h>
#include <util/stream/output.h>

int main(int argc, char **argv)
{
    --argc;
    ++argv;

    if (argc != 1){
        Cerr << "Usage: refdmap_index_print <indexfile>\n";
        return 1;
    }

    TXRefMappedArray data;
    data.Init(argv[0]);

    int count = data.Size();
    Cout << "Count: " << count << '\n';
    for (int i=0; i < count; ++i){
        const TDMapInfo& info = data[i].Info();
        Cout << (ui32)info.WordCountSum << ' ' << (ui32)info.UniqueTextCount << '\n';
    }

    return 0;
}
