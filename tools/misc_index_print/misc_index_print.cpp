/// author@ cheusov@ Aleksey Cheusov
/// created: Mon, 13 Oct 2014 14:42:38 +0300
/// see: OXYGEN-899

#include <library/cpp/on_disk/chunks/chunked_helpers.h>
#include <util/stream/output.h>

int main(int argc, char **argv)
{
    --argc;
    ++argv;

    if (argc != 1){
        Cerr << "Usage: misc_index_print <indexfile>\n";
        return 1;
    }

    TBlob blob(TBlob::FromFileSingleThreaded(argv[0]));
    TNamedChunkedDataReader data(blob);

    size_t blockCount = data.GetBlocksCount();
    Cout << "Block count:" << blockCount << '\n';

    for (size_t i=0; i < blockCount; ++i){
        size_t len = data.GetBlockLen(i);
        Cout << i << ": " << data.GetBlockName(i) << ' ' << len << '\n';
        const ui8* block = (const ui8*) data.GetBlock(i);
        assert(len == data.GetBlockLen(i));
        for (size_t j=0; j < len; ++j){
            Cout << ' ' << (ui32) block[j];
        }
        Cout << '\n';
    }

    return 0;
}
