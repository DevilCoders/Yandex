/// author@ cheusov@ Aleksey Cheusov
/// created: Tue, 14 Oct 2014 15:17:39 +0300
/// see: OXYGEN-891

#include <kernel/index_mapping/index_mapping.h>
#include <kernel/xref/xref_types.h>
#include <util/stream/output.h>

int main(int argc, char **argv)
{
    --argc;
    ++argv;

    if (argc != 1) {
        Cerr << "Usage: reflerf_index_print <indexfile>\n";
        return 1;
    }


    TLerfMapped2DArray data;
    if (!data.Init(argv[0])) {
        Cerr << "cannot load " << argv[0] << '\n';
        return 2;
    }

    size_t count = data.Size();
    Cout << "Count:" << count << '\n';
    for (size_t i = 0; i < count; ++i) {
        const ui8* beg = data.GetBegin(i);
        const ui8* end = data.GetEnd(i);
        Cout << i << " {" << end-beg << "} :";
        for (; beg < end; ++beg){
            Cout << ' ' << (ui32)*beg;
        }
        Cout << '\n';
    }

    return 0;
}
