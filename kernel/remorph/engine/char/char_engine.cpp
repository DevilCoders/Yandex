#include "char_engine.h"

#include "rules_parser.h"
#include "version.h"

#include <kernel/remorph/common/magic_input.h>
#include <kernel/remorph/common/verbose.h>
#include <kernel/remorph/core/core.h>

#include <util/generic/algorithm.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>

namespace NReMorph {

void TCharEngine::LoadFromFile(const TString& filePath, const TGazetteer*) {
    TIFStream fIn(filePath);
    TMagicInput in(fIn);
    if (in.HasMagic()) {
        REPORT(INFO, "Loading binary char-engine from '" << filePath << "'");
        try {
            in.Check(NMatcher::MT_CHAR, NPrivate::CHAR_BINARY_VERSION);
        } catch (TBinaryError& error) {
            error.SetFilePath(filePath);
            throw error;
        }
        TProfileTimer t;
        LoadFromStream(in, false);
    } else {
        REPORT(INFO, "Parsing char-engine from '" << filePath << "'");
        TStreamParseOptions parseOptions(in, filePath);
        Parse(parseOptions);
    }
    REPORT(DETAIL,
        "Minimal path: " << Dfa->MinimalPathLength << Endl
        << "Flags:"
        << (0 == Dfa->Flags ? " None" : "")
        << (0 != (Dfa->Flags & NRemorph::DFAFLG_STARTS_WITH_ANCHOR) ? " STARTS_WITH_ANCHOR" : "")
        << Endl
        << "Literals: " << LiteralTable->Size()
    );
    if (GetVerbosityLevel() > int(TRACE_DETAIL)) {
        PrintStats(GetInfoStream(), *Dfa);
    }
}

void TCharEngine::ParseFromString(const TString& rules) {
    TStringInput input(rules);
    TStreamParseOptions parseOptions(input);
    Parse(parseOptions);
}

void TCharEngine::CollectUsedGztItems(THashSet<TUtf16String>&) const {
}

void TCharEngine::SaveToStream(IOutputStream& out) const {
    ::WriteMagic(out, NMatcher::MT_CHAR, NPrivate::CHAR_BINARY_VERSION);
    LiteralTable->Save(out);
    ::Save(&out, Dfa);
    ::Save(&out, RuleNames);
}

void TCharEngine::LoadFromStream(IInputStream& in, bool signature) {
    if (signature) {
        TMagicCheck(in).Check(NMatcher::MT_CHAR, NPrivate::CHAR_BINARY_VERSION);
    }
    LiteralTable = new NPrivate::TLiteralTable();
    LiteralTable->Load(in);
    ::Load(&in, Dfa);
    ::Load(&in, RuleNames);
}

TCharEnginePtr TCharEngine::Load(const TString& filePath) {
    TCharEnginePtr engine(new TCharEngine());
    engine->LoadFromFile(filePath, nullptr);
    return engine;
}

TCharEnginePtr TCharEngine::Load(IInputStream& in) {
    TCharEnginePtr engine(new TCharEngine());
    engine->LoadFromStream(in, true);
    return engine;
}

TCharEnginePtr TCharEngine::Parse(const TString& rules) {
    TCharEnginePtr engine(new TCharEngine());
    engine->ParseFromString(rules);
    return engine;
}

void TCharEngine::Parse(const TStreamParseOptions& options) {
    NPrivate::TRulesParser rulesParser(options);
    NPrivate::TRulesParser::TResult parseResult = rulesParser.Parse();
    LiteralTable = parseResult.LiteralTable;
    DoSwap(RuleNames, parseResult.Rules);
    NRemorph::TNFAPtr nfa = NRemorph::Combine(parseResult.NFAs);
    REPORT(INFO, "Building DFA...");
    TProfileTimer t;
    Dfa = NRemorph::Convert(*LiteralTable, *nfa);
    REPORT(INFO, "Parsing took " << t.Get().MilliSeconds() << "ms");
}

} // NReMorph
