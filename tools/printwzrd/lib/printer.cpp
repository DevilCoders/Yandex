#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/strip.h>
#include <util/string/cast.h>

#include <search/wizard/core/wizardrule.h>
#include <search/wizard/face/rrr_printer.h>
#include <search/wizard/core/wizglue.h>

#include <ysite/yandex/reqdata/reqdata.h>
#include <kernel/qtree/request/fixreq.h>
#include <kernel/qtree/richrequest/nodeiterator.h>
#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/richrequest/printrichnode.h>

#include "options.h"
#include "printer.h"

TRequestsInfo::TRequestsInfo()
    : LemmaCount(0)
    , WordCount(0)
    , RequestCount(0)
{
}

void TRequestsInfo::Update(const TRichTreePtr& tree) {
    ++RequestCount;
    TWordIterator it(tree->Root);
    while (!it.IsDone()) {
        ++WordCount;
        if (it->WordInfo.Get())
            LemmaCount += it->WordInfo->GetLemmas().size();
        ++it;
    }
}

void TRequestsInfo::Update(const TRequestsInfo& other) {
    LemmaCount += other.LemmaCount;
    WordCount += other.WordCount;
    RequestCount += other.RequestCount;
}

void TRequestsInfo::Print(IOutputStream& out) const {
    out << "Request count: [" << RequestCount << "]" << Endl;
    out << "Word count: [" << WordCount << "]" << Endl;
    out << "Lemma count: [" << LemmaCount << "]" << Endl;

    out << "Average word count: [" << (double) WordCount / RequestCount << "]" << Endl;
    out << "Average lemma count: [" << (double) LemmaCount / RequestCount << "]" << Endl;
}

void TPrintwzrdPrinter::PrintRichTree(const NSearchQuery::TRequest* tree) {
    TString text;
    tree->SerializeToClearText(text);
    Out << text << Endl;
}

void TPrintwzrdPrinter::PrintRequestForSources(const TSourceResPtr& sr) {
    Y_ASSERT(sr.Get() != nullptr);
    unsigned sources_count = sr->SourceWithSpecialRequestCount();
    unsigned i = 0;
    for (i = 0; i < sources_count; ++i){
        TString source = sr->SourceWithSpecialRequestName(i);
        TString request;
        sr->GetRequestForSources(source, request);
        Out << "[" << source << "]: [" << request << "]" << Endl;
    }
}

void TPrintwzrdPrinter::PrintSuccessfulRules(const IRulesResults* rrr) {
    Y_ASSERT(rrr != nullptr);
    const unsigned rulesCount = rrr->GetRuleSize();
    NWiz::TRrrPrinter printer(rrr);
    Out << "Successful rules: [";
    for (unsigned ruleIndex = 0; ruleIndex < rulesCount; ++ruleIndex) {
        const TStringBuf ruleName = rrr->GetRuleName(ruleIndex);
        if (printer.IsUnsuccessfulRule(ruleName))
            continue;
        Out << ruleName << " ";
    }
    Out << "]" << Endl << Endl;
}

void TPrintwzrdPrinter::PrintProperties(const IRulesResults* rrr) {
   NWiz::TRrrPrinter printer(rrr);
   printer.IsDebugMode = Options.DebugMode;
   printer.IsSortedOutput = Options.SortOutput;
   printer.SetRulesToPrint(RulesToPrint);
   if (RulesToPrint.empty() || RulesToPrint.find("Gzt") != RulesToPrint.end())
       printer.PrintGazetteerResults(Out);
   printer.PrintProperties(Out);
}

void TPrintwzrdPrinter::PrintCgiParamRelev(const TCgiParameters& cgiParam) {

    TString cgiParamRelev = cgiParam.Get("relev");
    CGIUnescape(cgiParamRelev);

    Out << "cgiRelev: [" << NWiz::TRrrPrinter::PrintFactors(cgiParamRelev, Options.SortOutput, true) << "]" << Endl;

    TString cgiParamCatalogue = cgiParam.Get("catalogue");
    CGIUnescape(cgiParamCatalogue);
    Out << "cgiCatalogue: [" << cgiParamCatalogue<< "]" << Endl;

    TString foreignQtree = cgiParam.Get("foreign_qtree");
    if (!foreignQtree.empty()) {
        Out << "ForeignQtree: [" << foreignQtree << "]" << Endl;
    }

    TString qtree = cgiParam.Get("qtree");
    if (!qtree.empty()) {
        Out << "Qtree: [" << qtree << "]" << Endl;
    }
    TString qtreeOld = cgiParam.Get("qtree_old");
    if (!qtreeOld.empty()) {
        Out << "Qtree old: [" << qtreeOld << "]" << Endl;
    }

    Out << "cgiRearr: [" << NWiz::TRrrPrinter::PrintFactors(cgiParam.Get("rearr"), Options.SortOutput, false) << "]" << Endl;
    Out << "cgiSnippet: [" << cgiParam.Get("snip") << "]" << Endl;

    size_t numPronValues = cgiParam.NumOfValues("pron");
    for (size_t i = 0; i < numPronValues; ++i) {
        Out << "cgiPron: [" << cgiParam.Get("pron", i) << "]" << Endl;
    }
//    TString log = cgiParam.Get("wizard_log");
//    if (!log.empty()) {
//        Out << "WizardLog: [" << log << "]" << Endl;
//    }

}

void TPrintwzrdPrinter::PrintRequest(const TRichTreePtr& tree) {
    TString s = "Error";
    if (tree.Get())
        s = WideToUTF8(PrintRichRequest(tree.Get()));
    Out << "[search]: [" << s << "]" << Endl;
}

void TPrintwzrdPrinter::PrintQtree(const TBinaryRichTree& blob) {
    TString outs = EncodeRichTreeBase64(blob);;
    CGIEscape(outs);
    Out << "[qtree]: [" << outs.c_str() << "]"<<Endl;
}

namespace {
    struct TCount {
        size_t Value;
        TCount()
            : Value(0)
        {
        }
    };
}

void TPrintwzrdPrinter::SetPrintRules(TStringBuf rulesToPrint) {
    while (!rulesToPrint.empty()) {
        TStringBuf rule = StripString(rulesToPrint.NextTok(','));
        if (!rule.empty())
            RulesToPrint.insert(ToString(rule));
    }
}

void TPrintwzrdPrinter::Print(const TSourceResPtr& sr, const TWizardResPtr& wr, const TCgiParameters& cgi, const TString& request) {

    if (Options.PrintMarkup && wr.Get() != nullptr) {
        Out << wr->GetProperty("ExternalMarkup", "JSON", 0) << Endl;
        return;
    }

    bool hasRulesToPrint = !RulesToPrint.empty();

    const TString text = cgi.Get("text", 0);
    TString packedBlob = cgi.Get("qtree", 0);

    const TString req = cgi.Get("user_request", 0);

    if (Options.PrintSuccessfulRules && wr.Get() != nullptr) {
        Out << req << Endl;
        PrintSuccessfulRules(wr.Get());
    }

    if (Options.PrintSrc) {
        Out << "src: [" << request << "]" << Endl;
    }
    if (Options.PrintExtraOutput && wr.Get() != nullptr) {
        PrintProperties(wr.Get());
    }

    if (hasRulesToPrint) {
        Out << Endl;
        Out.Flush();
        return;
    }

    if (!text.empty() && !packedBlob.empty()) { //wizard answered
        CGIUnescape(packedBlob);
        TBlob packedRequestTree = DecodeRichTreeBase64(packedBlob);
        TRichTreePtr tree = DeserializeRichTree(packedRequestTree);

        if (Options.PrintExtraOutput && tree.Get() || Options.PrintQtree) {
            PrintRequest(tree);

            if (Options.PrintQtree)
                PrintQtree(packedRequestTree);
        }
        if (Options.PrintRichTree && tree.Get())
            PrintRichTree(tree.Get());

        if (Options.PrintLemmaCount)
            RequestsInfo.Update(tree);
    }
    if (Options.PrintExtraOutput && sr.Get() != nullptr)
        PrintRequestForSources(sr);

    if (Options.PrintExtraOutput || Options.PrintCgiParamRelev) {
        PrintCgiParamRelev(cgi);
    }

    if (Options.PrintExtraOutput) {
        Out << Endl;
    }

    Out.Flush();
}

const TRequestsInfo* TPrintwzrdPrinter::GetInfo() const {
    return Options.PrintLemmaCount ? &RequestsInfo : nullptr;
}

TAutoPtr<IWizardResultsPrinter> GetResultsPrinter(const TPrintwzrdOptions& options, IOutputStream& out) {
    if (options.PrintDolbilka)
        return new TUpperSearchPrinter(out);
    else
        return new TPrintwzrdPrinter(out, options);
}
