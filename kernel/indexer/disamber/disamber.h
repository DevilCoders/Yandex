#pragma once

#include <dict/disamb/zel_disamb/zel_disamb_model.h>
#include <kernel/indexer/face/directtext.h>
#include <kernel/indexer/direct_text/dt.h>
#include <util/generic/vector.h>

namespace NIndexerCore {

struct TDirectTextEntry2;


class TDefaultDisamber : public IDisambDirectText {
private:
    TDisambModel<DisambText> DisambModel;
public:
    TDefaultDisamber() {
    }
    void ProcessText(const TDirectTextEntry2* entries, size_t entCount, TVector<TDisambMask>* masks) override;
};

}
