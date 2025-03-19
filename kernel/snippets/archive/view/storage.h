#pragma once

#include <kernel/snippets/iface/archive/enums.h>
#include <kernel/snippets/iface/archive/sent.h>

#include <util/generic/string.h>

namespace NSnippets {
    class TArchiveStorage {
    private:
        TArchiveSentList Sents;
    public:
        TArchiveStorage();
        ~TArchiveStorage();
        TArchiveSentList::TIterator Add(EARC arc, int sentId, const TUtf16String& rawSent, ui16 sentFlags);
    };
}
