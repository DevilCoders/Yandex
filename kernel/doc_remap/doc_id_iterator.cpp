#include "doc_id_iterator.h"

void TDocIdIterator::ReadNextIt() {
    Eof = (Offset*4 >= Remap.getSize());
    if (!Eof) {
        CurrentRemap = *((ui32*)Remap.getData() + Offset);
        Current = Offset;
        ++Offset;
    }
}

void TDocIdIterator::ReadNext() {
    if (!Eof) {
        ReadNextIt();
        while (!Eof && ((ui32)-1 == CurrentRemap))
            ReadNextIt();
    }
}

TDocIdIterator::TDocIdIterator(const char* index)
    : Remap((TString(index) + "arr").data())
    , Offset(0)
    , Eof(false)
{
    ReadNext();
}

bool TDocIdIterator::IsEof() const {
    return Eof;
}

ui32 TDocIdIterator::GetCurrent() const {
    return Current;
}

ui32 TDocIdIterator::GetCurrentRemap() const {
    return CurrentRemap;
}

void TDocIdIterator::Next() {
    ReadNext();
}
