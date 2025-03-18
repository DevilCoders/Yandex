/// author@ cheusov@ Aleksey Cheusov
/// created: Thu,  9 Oct 2014 17:05:40 +0300
/// see: OXYGEN-904

#include <kernel/url_sequences/url_sequences.h>

int main(int argc, char **argv)
{
    --argc;
    ++argv;

    if (argc != 1){
        Cerr << "Usage: titleseq_index_print <indexfile>\n";
        return 1;
    }

    NSequences::TArray array(argv[0]);
    NSequences::TReader reader(array);

    for (size_t docId = 0; docId < array.GetSize(); ++docId) {
        if (!reader.CheckDoc(docId))
            continue;

        reader.InitDoc(docId);
        const NSequences::TEntry *e = reader.GetCurrDoc();
        Cout << docId << ": "
             << e->PrefixId << ' ' << (ui32)e->PrefixLen << ' '
             << (ui32)e->DomainLen << ' ' << (ui32)e->PathLen << ' '
             << reader.GetUrl() << '\n';
    }

    return 0;
}
