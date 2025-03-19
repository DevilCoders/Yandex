#include <kernel/qtree/richrequest/nodeiterator.h>

#include "helpers.h"


bool IsFullSubtraction(const TRichNodePtr& root) {
    THashSet<TUtf16String> l;
    for (TAllNodesIterator it(root); !it.IsDone(); ++it) {
        if (it->IsAndNotOp()) {
            it->CollectLemmas(l);
        }
    }
    return root->HasAnyLemma(l);
}


bool HasUserOperators(const TRichNodePtr& root) {
    for (TRichNodeIterator treeIter(root); !treeIter.IsDone(); ++treeIter) {
        if (treeIter->GetPhraseType() == PHRASE_USEROP && !treeIter->IsQuoted())
            return true;
    }
    return false;
}


bool HasUrlOperators(const TRichNodePtr& root) {
    for (TAttributeIterator it(root); !it.IsDone(); ++it) {
        const TUtf16String& attr = it->GetTextName();
        if (attr == URL_STR || attr == RHOST_STR || attr == HOST_STR ||
            attr == SITE_STR || attr == INURL_STR)
        {
            return true;
        }
    }
    return false;
}
