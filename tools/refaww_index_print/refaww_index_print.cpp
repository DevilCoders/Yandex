/// author@ cheusov@ Aleksey Cheusov
/// created: Sun, 12 Oct 2014 16:32:04 +0300
/// see: OXYGEN-887

#include <kernel/xref/xref_types.h>
#include <util/stream/output.h>

int main(int argc, char **argv)
{
    --argc;
    ++argv;

    if (argc != 1){
        Cerr << "Usage: refaww_index_print <indexfile>\n";
        return 1;
    }

    SearchAncWrdWht data;
    data.Init(argv[0]);

    int count = data.Size();
    Cout << "Count: " << count << '\n';
    for (int i=0; i < count; ++i){
        Cout << data[i] << '\n';
    }

    return 0;
}
