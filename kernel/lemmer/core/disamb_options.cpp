#include "disamb_options.h"
#include "morpho_lang_discr.h"

TDisambiguationOptions::TDisambiguationOptions()
    : FilterLanguages(true)
    , FilterOneWordLemmas(true)
    , DiscriminatorFixList(&NMorphoLangDiscriminator::TContext::EmptyContext)
{
}

const NMorphoLangDiscriminator::TContext& TDisambiguationOptions::GetDiscrList() const {
    Y_ASSERT(DiscriminatorFixList);
    return *DiscriminatorFixList;
}

void TDisambiguationOptions::SetDiscrList(const NMorphoLangDiscriminator::TContext& dl) {
    DiscriminatorFixList = &dl;
}
