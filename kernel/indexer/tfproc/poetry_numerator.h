#pragma once

#include "poetry.h"
#include <library/cpp/html/face/parsface.h>
#include <library/cpp/numerator/numerate.h>
#include <library/cpp/token/nlptypes.h>
#include <kernel/indexer/face/inserter.h>
#include <util/charset/wide.h>
#include <util/string/cast.h>
#include <library/cpp/html/spec/tags.h>

// Class for translate parser events to PoetryMatcher
class TPoetryHandler : public INumeratorHandler {
private:
    TPoetryMatcher PoetryMatcher;               // Poetry calculator
    TWordForces *WordForces;                    // Global list with words' forces
    size_t PreDeep;                             // <pre> and etc. deep

public:
    TPoetryHandler()
        : WordForces(nullptr)
        , PreDeep(0)
    {
    }

    void SetWordForces(TWordForces *word_forces) {
        WordForces = word_forces;
    }
    // Process text token
    void OnTokenStart(const TWideToken& tok, const TNumerStat&) override {
        TVector<float> forces;
        WordForces->GetForces(tok, &forces);
        PoetryMatcher.AddWord(forces);
    }
    // Process tag
    void OnMoveInput(const THtmlChunk& chunk, const TZoneEntry*, const TNumerStat&) override {
        ProcessMarkup(chunk);
    }
    // Process line break if we are in <pre></pre>
    void OnSpaces(TBreakType , const wchar16* token, unsigned len, const TNumerStat&) override {
        if (PreDeep) {
            if (TWtringBuf{token, len}.Contains('\n'))
                FlushLine();
        }
    }
    // Get the poetry value
    float PoetryValue() {
        FlushLine();
        return PoetryMatcher.PoetryValue();
    }
    // Get the poetry value of the part of the document
    float PoetryValue2() {
        FlushLine();
        return PoetryMatcher.PoetryValue2();
    }

    void InsertFactors(IDocumentDataInserter& inserter) {
        float val = PoetryValue();
        if (val > 1e-5)
            inserter.StoreErfDocAttr("Poetry", ToString(val));
        val = PoetryValue2();
        if (val > 1e-5)
            inserter.StoreErfDocAttr("Poetry2", ToString(val));
    }

private:
    void ProcessMarkup(const THtmlChunk& c) {
        int type = c.flags.type;
        if (PARSED_MARKUP != type)
            return;
        if (MARKUP_IGNORED == c.flags.markup) //the tag is discarded
            return;
        Y_ASSERT(c.Tag);

        if (c.Tag->is(HT_br)) {
            FlushLine();
            if (c.Tag->is(HT_TD) || c.Tag->is(HT_TH))
                PoetryMatcher.OnZoneBreak();
        }

        if (c.Tag->is(HT_pre)) {
            if (c.GetLexType() == HTLEX_START_TAG)
                ++PreDeep;
            else
                --PreDeep;
        }

        // Store information about anchor presence
        if (c.Tag->is(HT_A)) {
            static const TVector<float> dumb(20);
            PoetryMatcher.AddWord(dumb);
        }
    }

    void FlushLine() {
        PoetryMatcher.FlushLine();
    }
};
