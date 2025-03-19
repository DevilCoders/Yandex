#pragma once

#include <kernel/snippets/replace/replace.h>

#include <util/generic/string.h>

namespace NSnippets {

extern const TUtf16String MEDIA_WIKI_PARA_MARKER;

class TMediaWikiReplacer : public IReplacer {
public:
    TMediaWikiReplacer();
    void DoWork(TReplaceManager* manager) override;
};

} // namespace NSnippets
