#include <kernel/url_sequences/url_sequences.h>

#include <library/cpp/getopt/opt.h>

static void Usage() {
    puts("Usage:");
    puts(" -i : url sequence file (index.urlseq default)");
    puts(" -v : verbose mode");
}

int DoMain(int argc, char* argv[]) {
    TString indexUrlSeq = "index.urlseq";
    bool verbose = false;
    ui32 docId = (ui32)-1;

    try {
        OPTION_HANDLING_PROLOG_ANON("hi:vd:");
        OPTION_HANDLE('h', (Usage(), exit(1)));
        OPTION_HANDLE('i', (indexUrlSeq = opt.Arg));
        OPTION_HANDLE('v', (verbose = true));
        OPTION_HANDLE('d', (docId = FromString<ui32>(opt.Arg)));
        OPTION_HANDLING_EPILOG;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        Usage();
        return 1;
    }

    NSequences::TArray urlArray(indexUrlSeq);
    NSequences::TReader urlSeqReader(urlArray);
    bool warned = false;
    ui32 begin = 0;
    ui32 end = urlArray.GetSize() - 1;
    if (docId != (ui32)-1) {
        begin = docId;
        end = docId;
    }
    for (ui32 docId = begin; docId <= end; ++docId) {
        if (urlArray.GetBegin(docId) == urlArray.GetEnd(docId)) {
            if (!warned) {
                printf("Warning: there are empty spaces in %s, starting with %i.\n", indexUrlSeq.data(), (int)docId);
                warned = true;
            }
            continue;
        }
        urlSeqReader.InitDoc(docId);
        const NSequences::TEntry* CurrDoc = urlSeqReader.GetCurrDoc();
        printf("%i\t", (int)docId);
        if (verbose)
            printf("%i\t%i\t", (int)CurrDoc->PrefixId, (int)CurrDoc->PrefixLen);
        printf("%i\t%i\t%s.\n", (int)CurrDoc->DomainLen, (int)CurrDoc->PathLen, urlSeqReader.GetUrl().data());
    }
    return 0;
}

int main(int argc, char **argv) {
    try {
        return DoMain(argc, argv);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
