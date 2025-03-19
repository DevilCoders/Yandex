#include "storage.h"

namespace NSnippets {

    TArchiveStorage::TArchiveStorage() {
    }

    TArchiveStorage::~TArchiveStorage() {
    }

    TArchiveSentList::TIterator TArchiveStorage::Add(EARC arc, int sentId, const TUtf16String& rawSent, ui16 sentFlags) {
        Sents.PushBack(new TArchiveSent(arc, sentId, rawSent, sentFlags));
        return --Sents.End();
    }
}
