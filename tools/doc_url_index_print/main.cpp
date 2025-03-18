/// author@ vvp@ Victor Ploshikhin
/// created: Oct 25, 2011 7:50:02 PM
/// see: BUKI-1289

#include <kernel/doc_url_index/doc_url_index.h>

int main(int /*argc*/, char** argv)
{
    TDocUrlIndexReader reader(argv[1]);

    for ( size_t i = 0; i < reader.Size(); ++i ) {
        Cout << i << ":\t" << reader.Get(i) << "\n";
    }

    return 0;
}

