#pragma once

#include "url_functions.h"

#include <kernel/segmentator/structs/spans.h>

#include <library/cpp/html/face/event.h>
#include <library/cpp/html/face/propface.h>
#include <library/cpp/html/face/zoneconf.h>
#include <library/cpp/html/spec/tags.h>
#include <library/cpp/token/nlptypes.h>
#include <library/cpp/token/token_structure.h>

namespace NSegm {

class IStorer {
protected:
    const IParsedDocProperties* Parser = nullptr;
public:
    virtual ~IStorer() {}

    void SetParser(const IParsedDocProperties* p) {
        Parser = p;
    }

    // numerator events

    virtual void OnEvent(bool /*title*/, const THtmlChunk&, const TZoneEntry*, TAlignedPosting) {
    }

    virtual void OnToken(bool /*title*/, const TWideToken&, TAlignedPosting) {
    }

    virtual void OnSpaces(bool /*title*/, TBreakType, const wchar16*, unsigned, TAlignedPosting) {
    }

    virtual void OnTextEnd(TAlignedPosting) {
    }

    // tree builder events

    virtual void OnLinkOpen(const TString& /*url*/, ELinkType /*ext*/, TAlignedPosting) {}
    virtual void OnLinkClose(TAlignedPosting) {}
    virtual void OnBlockOpen(HT_TAG, TAlignedPosting) {}
    virtual void OnBlockClose(HT_TAG, TAlignedPosting) {}
    virtual void OnBreak(HT_TAG, TAlignedPosting) {}
    virtual void OnInput(TAlignedPosting) {}
    virtual void OnImage(const TString& /*src*/, const TString& /*alt*/, ELinkType /*ext*/, TAlignedPosting) {}
};

}
