#pragma once

#include <library/cpp/containers/comptrie/comptrie.h>

#include <util/generic/vector.h>

namespace NLemmasMerger {

class TWordsTrie : public TCompactTrie<ui32, ui32> {
public:
    void Minimize(TVector<ui32>& text) const {
        ui32* writePointer = text.begin();
        ui32* readPointer = text.begin();
        ui32* end = text.end();

        while (readPointer < end) {
            size_t preffixLen = 0;
            const char* valuepos = nullptr;

            if (!LookupLongestPrefix(readPointer, end - readPointer, preffixLen, valuepos)) {
                *writePointer = *readPointer;

                ++writePointer;
                ++readPointer;
                continue;
            }

            ui32 wordNumber = 0;
            Packer.UnpackLeaf(valuepos, wordNumber);

            *writePointer = wordNumber;
            readPointer += preffixLen;
            ++writePointer;
        }

        text.crop(writePointer - text.begin());
    }
};

}
