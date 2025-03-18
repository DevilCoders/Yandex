/// @author Victor Ploshikhin
/// created May 25, 2011 5:24:07 PM

#include <kernel/doc_remap/remap_reader.h>

#include <ysite/yandex/trigram_index/trigram_index.h>

#include <library/cpp/getopt/opt2.h>

int main(int argc, char** argv)
{
    Opt2 opt(argc, argv, "i:s:");

    const TString indexFile = opt.Arg('i', "<file> - path to index", "");
    size_t offsetSize = static_cast<size_t>(opt.UInt('s', "<offsetSize> - trigram offset", 32));

    opt.AutoUsageErr();

    TTrigramIndexReader index(indexFile, offsetSize);
    for ( TTrigramIndexReadIterator it = TTrigramIndexReadIterator(index); it.IsValid(); it.Next() ) {
        TTrigramIndexReadIterator::TData data = it.GetValue();

        //ui32 archDocId = 0;

        Cout << data.DocId << " : ";
        for ( size_t i = 0; i < data.NumElems; ++i ) {
            Cout << TString((const char*)data.Trigrams[i].GetData(), 3);
            if ( i + 1 != data.NumElems ) {
                Cout << ",";
            }
        }
        Cout << "\n";
    }
    return 0;
}
