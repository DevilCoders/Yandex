#include <kernel/lemmer/superlemmer/superlemmer.h>
#include <util/memory/blob.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/stream/input.h>
#include <util/stream/output.h>

using NSuperLemmer::TSuperLemmer;

void Usage() {
    Cerr << "Usage: ./superlemmer-test [trie.bin]\n";
    Cerr << "(by default the built-in trie.bin will be used)\n";
    Cerr << "Input: lemmas\n";
    Cerr << "Output: superlemmas\n";
}

int main(int argc, char* argv[]) {
    THolder<TSuperLemmer> superLemmer;
    if (argc == 1) {
        superLemmer.Reset(new TSuperLemmer());
    } else if (argc == 2) {
        try {
            TBlob blob = TBlob::PrechargedFromFile(argv[1]);
            superLemmer.Reset(new TSuperLemmer(blob));
        } catch (...) {
            Usage();
            return 1;
        }
    } else {
        Usage();
        return 1;
    }
    TString key;
    while (Cin.ReadLine(key)) {
        Cout << key << ":\t" << superLemmer->Apply(key) << Endl;
    }
}
