#include "superlemmer.h"

#include <library/cpp/archive/yarchive.h>

namespace {
    extern "C" {
    extern const unsigned char TrieBin[];
    extern const ui32 TrieBinSize;
    };
}

using namespace NSuperLemmer;

TSuperLemmer::TSuperLemmer() {
    TArchiveReader reader(TBlob::NoCopy(TrieBin, TrieBinSize));
    auto key = reader.KeyByIndex(0);
    Data = reader.ObjectBlobByKey(key);
    Trie.Init(Data);
}

TSuperLemmer::TSuperLemmer(TBlob data)
    : Data(data)
{
    Trie.Init(Data);
}
