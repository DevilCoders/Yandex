#include "hititeratorheap.h"

size_t THitIteratorHeap::NextLemma() {
    if (!CurrentHeapSize)
        return 0;
    Forms.PrepareForNextLemma();
    for (size_t n = 0; n < CurrentSize; n++)
        BaseAddress[(*this)[n]->ArrIndex].HiTitersCount = 0;
    bool isFirstLemma = !CurrentSize;
    CurrentSize = 0;
    AdditionalIteratorsUsed = 0;
    TNumFormArray formIndexes;
    if (!isFirstLemma) {
        Forms.FindFormIndexes(formIndexes);
        AddIterators(formIndexes);
    }
    while (!Forms.ProcessNextKey(TheHeap[0]->Str()) || isFirstLemma) {
        if (isFirstLemma) {
            Forms.PrepareForNextLemma();
            isFirstLemma = false;
        }
        Forms.FindFormIndexes(formIndexes);
        if (!AddIterators(formIndexes))
            break;
    }
    return CurrentSize;
}

