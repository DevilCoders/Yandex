#include "reader.h"

namespace NIndexAnn {

void IDocDataIterator::AnnounceDocIds(TConstArrayRef<ui32> /* docIds */) {
    Y_FAIL("Unimplemented");
}

void IDocDataIterator::PreLoadDoc(ui32 /* docId */, const NDoom::TSearchDocLoader& /* loader */) {
    Y_FAIL("Unimplemented");
}

bool IDocDataIterator::HasDoc(ui32 /* doc */) {
    Y_FAIL("Unimplemented");
}

THolder<IDocDataIterator> IDocDataIterator::Clone(const IDocDataIndex* index) {
    return index->CreateIterator();
}

}
