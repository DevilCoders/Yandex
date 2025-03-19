#include "remorph_matcher.h"
#include "rule_parser.h"
#include "version.h"

#include <kernel/remorph/common/magic_input.h>
#include <kernel/remorph/common/verbose.h>
#include <kernel/remorph/core/core.h>

#include <util/datetime/cputimer.h>
#include <util/generic/algorithm.h>
#include <util/stream/file.h>

namespace NReMorph {

void TMatcher::LoadFromFile(const TString& filePath, const TGazetteer* gzt) {
    TIFStream fIn(filePath);
    TMagicInput in(fIn);
    if (in.HasMagic()) {
        REPORT(INFO, "Loading binary remorph from '" << filePath << "'");
        try {
            in.Check(NMatcher::MT_REMORPH, REMORPH_BINARY_VERSION);
        } catch (TBinaryError& error) {
            error.SetFilePath(filePath);
            throw error;
        }
        TProfileTimer t;
        LoadFromStream(in, false);
        REPORT(INFO, "Binary loading took " << t.Get().MilliSeconds() << "ms");
    } else {
        REPORT(INFO, "Parsing remorph from '" << filePath << "'");
        NPrivate::TParseResult parseResult;
        ParseFile(parseResult, filePath, in);
        LiteralTable = parseResult.LiteralTable;
        DoSwap(RuleNames, parseResult.Rules);
        DoSwap(ExternalArticles, parseResult.UseGzt);
        NRemorph::TNFAPtr nfa = NRemorph::Combine(parseResult.NFAs);
        REPORT(INFO, "Building DFA...");
        TProfileTimer t;
        Dfa = NRemorph::Convert(*LiteralTable, *nfa);
        MinimalPathLength = Dfa->MinimalPathLength;
        REPORT(INFO, "Parsing took " << t.Get().MilliSeconds() << "ms");
    }
    REPORT(DETAIL,
        "Minimal path: " << Dfa->MinimalPathLength << Endl
        << "Flags:"
        << (0 == Dfa->Flags ? " None" : "")
        << (0 != (Dfa->Flags & NRemorph::DFAFLG_STARTS_WITH_ANCHOR) ? " STARTS_WITH_ANCHOR" : "")
        << Endl
        << "Literals: " << LiteralTable->Size() << ", sizeof=" << sizeof(NRemorph::TLiteral)
    );
    if (GetVerbosityLevel() > int(TRACE_DETAIL)) {
        PrintStats(GetInfoStream(), *Dfa);
    }
    Init(gzt);
}

void TMatcher::ParseFromArcadiaStream(IInputStream& stream, const NGzt::TGazetteer* gzt) {
    NPrivate::TParseResult parseResult;
    ParseStream(parseResult, stream);
    LiteralTable = parseResult.LiteralTable;
    DoSwap(RuleNames, parseResult.Rules);
    DoSwap(ExternalArticles, parseResult.UseGzt);
    NRemorph::TNFAPtr nfa = NRemorph::Combine(parseResult.NFAs);
    Dfa = NRemorph::Convert(*LiteralTable, *nfa);
    MinimalPathLength = Dfa->MinimalPathLength;
    Init(gzt);
}

void TMatcher::CollectUsedGztItems(THashSet<TUtf16String>& gztItems) const {
    LiteralTable->CollectUsedGztItems(gztItems);
    gztItems.insert(ExternalArticles.begin(), ExternalArticles.end());
}

void TMatcher::SaveToStream(IOutputStream& out) const {
    ::WriteMagic(out, NMatcher::MT_REMORPH, REMORPH_BINARY_VERSION);
    LiteralTable->Save(out);
    ::Save(&out, Dfa);
    ::Save(&out, RuleNames);
    ::Save(&out, ExternalArticles);
}

void TMatcher::LoadFromStream(IInputStream& in, bool signature) {
    if (signature) {
        TMagicCheck(in).Check(NMatcher::MT_REMORPH, REMORPH_BINARY_VERSION);
    }
    LiteralTable = new NLiteral::TLiteralTable();
    LiteralTable->Load(in);
    ::Load(&in, Dfa);
    ::Load(&in, RuleNames);
    ::Load(&in, ExternalArticles);
    MinimalPathLength = Dfa->MinimalPathLength;
}

TMatcherPtr TMatcher::Load(const TString& filePath, const NGzt::TGazetteer* gzt) {
    TMatcherPtr matcher(new TMatcher());
    matcher->LoadFromFile(filePath, gzt);
    return matcher;
}

TMatcherPtr TMatcher::Load(IInputStream& in, const NGzt::TGazetteer* gzt) {
    TMatcherPtr matcher(new TMatcher());
    matcher->LoadFromStream(in, true);
    matcher->Init(gzt);
    return matcher;
}

TMatcherPtr TMatcher::Load(IInputStream& in) {
    TMatcherPtr matcher(new TMatcher());
    matcher->LoadFromStream(in, true);
    return matcher;
}

TMatcherPtr TMatcher::Parse(const TString& rules, const NGzt::TGazetteer* gzt) {
    TMatcherPtr matcher(new TMatcher());
    matcher->ParseFromString(rules, gzt);
    return matcher;
}

TMatcherPtr TMatcher::Parse(IInputStream& rules, const NGzt::TGazetteer* gzt) {
    TMatcherPtr matcher(new TMatcher());
    matcher->ParseFromArcadiaStream(rules, gzt);
    return matcher;
}

} // NReMorph
