#include "antiddos_tool.h"
#include "request_iterator.h"

#include <antirobot/daemon_lib/cbb.h>
#include <antirobot/daemon_lib/factors.h>
#include <antirobot/daemon_lib/fullreq_info.h>
#include <antirobot/daemon_lib/match_rule_parser.h>

#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/json/writer/json.h>

#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/string/subst.h>


using namespace NAntiRobot;
using namespace NAntirobotEvClass;
using namespace NAntiRobot::NMatchRequest;


struct TStat {
    size_t Total = 0;
    size_t Matched = 0;

    TString Print(bool asJson) const {
        if (!asJson) {
            TStringStream str;
            str << "Total: " << Total << Endl
                << "Matched: " << Matched << Endl
                ;

            return str.Str();
        }

        NJsonWriter::TBuf json;
        json.BeginObject()
            .WriteKey("total")
            .WriteInt(Total)
            .WriteKey("matched")
            .WriteInt(Matched)
            .EndObject();
        return json.Str();
    }
};

TVector<TString> ReadRules(const TString& fileName) {
    TVector<TString> lines;
    if (fileName == "-") {
        Split(Cin.ReadAll(), "\n", lines);
    } else {
        TUnbufferedFileInput inp(fileName);
        Split(inp.ReadAll(), "\n", lines);
    }

    for (auto& s : lines) {
        s = StripInPlace(s);
    }

    return lines;
}

Y_DECLARE_UNUSED
inline void PrintErrors(const TVector<TString>& errorList) {
    Cout << JoinRange("\n", errorList.begin(), errorList.end()) << Endl;
}

bool CheckRules(const TVector<TString>& txtRules) {
    TVector<TString> errors;
    NAntiRobot::TPreparedRule::ParseList(txtRules, &errors);
    return errors.empty();
}

bool CheckRules(const TString& rules) {
    TVector<TString> txtRules = {rules};
    return CheckRules(txtRules);
}

bool CheckRules(const TSettings& settings) {
    const auto txtRules = ReadRules(settings.RulesFile);
    return CheckRules(txtRules);
}

TRuleSet ParseRules(const TVector<TString>& txtRules) {
    TVector<TString> errors;
    TRuleSet result = TRuleSet(
        {{TCbbGroupId{0}, NAntiRobot::TPreparedRule::ParseList(txtRules, &errors)}}
    );
    if (!errors.empty()) {
        throw yexception() << JoinRange("\n", errors.begin(), errors.end());
    }

    return result;
}

void ClearExistRulesOnCBB(const TString& cbbHost, TCbbGroupId cbbFlag) {
    TAntirobotTvm tvm(false, "");
    TCbbIO cbb(TCbbIO::TOptions{CreateHostAddr(cbbHost), DEFAULT_CBB_TIMEOUT, nullptr}, &tvm);

    auto wasRead = cbb.ReadTextList(cbbFlag);
    wasRead.Wait();

    if (!wasRead.HasValue() || !wasRead.GetValue()) {
        throw yexception() << "Could not read existing rules";
    }

    for (const auto& line : *wasRead.GetValue()) {
        TFuture<void> res = cbb.RemoveTxtBlock(cbbFlag, line);
        res.Wait();
        if (!res.HasValue()) {
            throw yexception() << "Could not erase existing rule";
        }
    }
}

void StoreRulesToCBB(const TString& cbbHost, TCbbGroupId cbbFlag, const TVector<TString>& rulesLines) {
    TAntirobotTvm tvm(false, "");
    TCbbIO cbb(TCbbIO::TOptions{CreateHostAddr(cbbHost), DEFAULT_CBB_TIMEOUT, nullptr}, &tvm);
    for (const auto& rule : rulesLines) {
        TString s = rule;
        SubstGlobal(s, ";", "%3B");
        TFuture<void> res = cbb.AddTxtBlock(cbbFlag, s, TString());
        res.Wait();
        if (!res.HasValue()) {
            Cerr << "Could not store rule: {" << rule << "}";
        }
    }
}

bool ValidateRequest(const TString& request) {
    const TRequestClassifier classifier;

    try {
        const auto req =  CreateDummyParsedRequest(request, classifier);
        return true;
    } catch (...) {
        return false;
    }
}

bool CheckRequest(const TString& rules, const TString& request) {
    TVector<TString> txtRules = {rules};
    auto ruleSet = ParseRules(txtRules);

    const TRequestClassifier classifier;

    const auto req =  CreateDummyParsedRequest(request, classifier);

    return ruleSet.Matches(*req);
}

void CheckSavedReqsWithRules(const TSettings& settings) {
    auto ruleSet = ParseRules(ReadRules(settings.RulesFile));

    auto reqIterator = settings.ReqLogTxt ? CreateTextLogIterator(settings.RequestLogName)
                                            : CreateEventLogIterator(settings.RequestLogName);

    TStat stat;

    while (const auto& req = reqIterator->Next()) {
        stat.Total++;
        if (ruleSet.Matches(*req)) {
            stat.Matched++;

            if (!settings.PrintMatched) {
                continue;
            }

            if (settings.PrintFullReq) {
                req->PrintData(Cout, /* forceMaskCookies := */ false);
                Cout << Endl;
            } else {
                Cout << req->Scheme << req->HostWithPort << req->RequestString << Endl;
            }
        }
    }

    if (settings.PrintStat) {
        Cout << stat.Print(settings.JsonOutput);
    }
}

void StoreNewRules(const TSettings& settings) {
    TVector<TString> rulesLines = ReadRules(settings.RulesFile);

    TVector<TString> errors;
    auto rules = ParseRules(rulesLines);

    if (settings.ClearExistRules) {
        Cerr << "Clearing existing rules...";
        ClearExistRulesOnCBB(settings.CbbHost, settings.CbbFlag);
        Cerr << "done" << Endl;
    }
    Cerr << "Storing new rules...";
    StoreRulesToCBB(settings.CbbHost, settings.CbbFlag, rulesLines);
    Cerr << "done" << Endl;
}

TString MakeJson(bool success, const TString& message) {
    NJsonWriter::TBuf json;
    json.BeginObject()
        .WriteKey("success")
        .WriteBool(success)
        .WriteKey("message")
        .WriteString(message)
        .EndObject();

    return json.Str();
}

TString PrintOK(bool asJson, const TString& msg) {
    if (!asJson) {
        return msg;
    }

    return MakeJson(true, msg);
}

TString PrintError(bool asJson, const TString& msg) {
    if (!asJson) {
        return msg;
    }

    return MakeJson(false, msg);
}
