#include "tlmatcher.h"
#include "rule_parser.h"
#include "version.h"

#include <kernel/remorph/common/verbose.h>
#include <kernel/remorph/common/magic_input.h>

#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/ysaveload.h>

namespace NTokenLogic {

void TMatcher::LoadFromFile(const TString& filePath, const NGzt::TGazetteer* gzt) {
    TIFStream fIn(filePath);
    TMagicInput in(fIn);
    if (in.HasMagic()) {
        REPORT(INFO, "Loading binary tokenlogic from '" << filePath << "'");
        try {
            in.Check(NMatcher::MT_TOKENLOGIC, TLOGIC_BINARY_VERSION);
        } catch (TBinaryError& error) {
            error.SetFilePath(filePath);
            throw error;
        }
        LoadFromStream(in, false);
    } else {
        REPORT(INFO, "Parsing tokenlogic from '" << filePath << "'");
        NTokenLogic::NPrivate::TParseResult parseResult = NTokenLogic::NPrivate::ParseRules(filePath, in);
        LiteralTable = parseResult.LiteralTable;
        DoSwap(Rules, parseResult.Rules);
        for (TVector<TString>::const_iterator i = parseResult.UseGzt.begin(); i != parseResult.UseGzt.end(); ++i) {
            ExternalArticles.push_back(UTF8ToWide(*i));
        }
    }
    if (GetVerbosityLevel() >= TRACE_DETAIL) {
        REPORT(DETAIL, "Loaded rules:");
        for (size_t i = 0; i < Rules.size(); ++i) {
            REPORT(DETAIL, Rules[i].ToString(*LiteralTable));
        }
    }
    Init(gzt);
}

void TMatcher::ParseFromString(const TString& rules, const NGzt::TGazetteer* gzt) {
    NTokenLogic::NPrivate::TParseResult parseResult = NTokenLogic::NPrivate::ParseRules(rules);
    LiteralTable = parseResult.LiteralTable;
    DoSwap(Rules, parseResult.Rules);
    for (TVector<TString>::const_iterator i = parseResult.UseGzt.begin(); i != parseResult.UseGzt.end(); ++i) {
        ExternalArticles.push_back(UTF8ToWide(*i));
    }
    if (GetVerbosityLevel() >= TRACE_DETAIL) {
        REPORT(DETAIL, "Loaded rules:");
        for (size_t i = 0; i < Rules.size(); ++i) {
            REPORT(DETAIL, Rules[i].ToString(*LiteralTable));
        }
    }
    Init(gzt);
}

void TMatcher::CollectUsedGztItems(THashSet<TUtf16String>& gztItems) const {
    LiteralTable->CollectUsedGztItems(gztItems);
    gztItems.insert(ExternalArticles.begin(), ExternalArticles.end());
}

void TMatcher::LoadFromStream(IInputStream& in, bool signature) {
    if (signature) {
        TMagicCheck(in).Check(NMatcher::MT_TOKENLOGIC, TLOGIC_BINARY_VERSION);
    }
    LiteralTable = new NLiteral::TLiteralTable();
    LiteralTable->Load(in);
    ::Load(&in, Rules);
    ::Load(&in, ExternalArticles);
}

void TMatcher::SaveToStream(IOutputStream& out) const {
    ::WriteMagic(out, NMatcher::MT_TOKENLOGIC, TLOGIC_BINARY_VERSION);
    LiteralTable->Save(out);
    ::Save(&out, Rules);
    ::Save(&out, ExternalArticles);
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

} // NTokenLogic
