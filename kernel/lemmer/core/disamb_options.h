#pragma once

#include <library/cpp/langmask/langmask.h>

namespace NMorphoLangDiscriminator {
    class TContext;
}

class TDisambiguationOptions {
public:
    TLangMask PreferredLangMask;
    TLangMask KeepLangMask;
    bool FilterLanguages;
    bool FilterOneWordLemmas;

    TDisambiguationOptions();

    const NMorphoLangDiscriminator::TContext& GetDiscrList() const;
    void SetDiscrList(const NMorphoLangDiscriminator::TContext& dl);

private:
    const NMorphoLangDiscriminator::TContext* DiscriminatorFixList;
};
