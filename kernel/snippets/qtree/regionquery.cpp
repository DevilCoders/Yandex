#include "regionquery.h"

#include <kernel/qtree/richrequest/nodeiterator.h>
#include <kernel/qtree/richrequest/richnode.h>

namespace NSnippets {

static void AddForm(TRegionQuery& ctx, const TUtf16String& form) {
    TUtf16String lowerForm = form;
    lowerForm.to_lower();
    ctx.Form2Positions[lowerForm].push_back(ctx.PosCount - 1);
}

static void OnWordNode(const TRichRequestNode& node, TRegionQuery& ctx) {
    ++ctx.PosCount;
    if (!node.WordInfo || !node.WordInfo->IsLemmerWord()) {
        AddForm(ctx, node.GetText());
    } else {
        for (const TLemmaForms& lemma : node.WordInfo->GetLemmas()) {
            for (const auto& form : lemma.GetForms()) {
                AddForm(ctx, form.first);
            }
            ctx.LangMask.SafeSet(lemma.GetLanguage());
        }
    }
}

TRegionQuery::TRegionQuery(const TRichRequestNode* tree) {
    if (!tree) {
        return;
    }
    typedef TTIterator<TForwardChildrenTraversal, IsWord, true> TUserWordsConstIterator;
    for (TUserWordsConstIterator it(*tree); !it.IsDone(); ++it) {
        OnWordNode(*it, *this);
    }
}

} // namespace NSnippets
