#include "request_classifier.h"

#include "fullreq_info.h"

#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/string/strip.h>

namespace NAntiRobot {
    TRegexpClassifierBuilder::TRegexpClassifierPtr
    TRegexpClassifierBuilder::CreateFromFile(const TString& filename, EReqType defaultReqType) {
        return CreateFromLines(ReadAllRules(filename), defaultReqType);
    }

    TRegexpClassifierBuilder::TRegexpClassifierPtr
    TRegexpClassifierBuilder::CreateFromLines(const TVector<TString>& lines, EReqType defaultReqType) {
        TVector<TRegexpClassifier::TClassDescription> rules;
        rules.reserve(lines.size());
        for (const auto& line : lines) {
            TVector<TString> ruleTokens;
            StringSplitter(line).SplitBySet("\t ").SkipEmpty().Collect(&ruleTokens);

            if (ruleTokens.size() < 2) {
                ythrow yexception() << "Cannot parse classifier rule: " << line;
            }

            EReqType reqType = FromString(ruleTokens.back());
            if (ruleTokens.size() == 2) {
                rules.emplace_back(GetFullPattern(ruleTokens[0]), reqType);
            } else {
                rules.emplace_back(GetFullPatternWithCgi(ruleTokens[0], ruleTokens[1]), reqType);
            }
        }
        return MakeSimpleShared<TRegexpClassifier>(rules.begin(), rules.end(), defaultReqType);
    }

    TString TRegexpClassifierBuilder::GetFullPattern(const TString& doc) {
        return doc + "\\?.*";
    }

    TString TRegexpClassifierBuilder::GetFullPatternWithCgi(const TString& doc, const TString& cgi) {
        return doc + "\\?" + cgi;
    }

    TString TRegexpClassifierBuilder::GetFullPatternWithHost(const TString& host, const TString& doc) {
        return host + doc + "\\?.*";
    }

    TString TRegexpClassifierBuilder::GetFullPattern(const TString& host, const TString& doc, const TString& cgi) {
        return host + doc + "\\?" + cgi;
    }

    TVector<TString> TRegexpClassifierBuilder::ReadAllRules(const TString& filename) {
        TFileInput input(filename);
        TVector<TString> lines;
        TString line;

        while (input.ReadLine(line)) {
            line = StripString(line);
            if (line.empty() || line.StartsWith('#')) {
                continue;
            }
            lines.emplace_back(line);
        }

        return lines;
    }

    TRequestClassifier TRequestClassifier::CreateFromRules(const TString& rulesPath) {
        TVector<std::pair<EHostType, TRequestClassifier::TRegexpClassifierPtr>> descriptions;

        for (size_t hostTypeIdx = HOST_TYPE_FIRST; hostTypeIdx < HOST_TYPE_LAST; ++hostTypeIdx) {
            EHostType hostType = static_cast<EHostType>(hostTypeIdx);
            auto filePath = TFsPath(rulesPath) / ToString(hostType);
            if (filePath.Exists()) {
                descriptions.emplace_back(hostType, TRegexpClassifierBuilder::CreateFromFile(filePath, REQ_OTHER));
            }
        }

        return TRequestClassifier::Create(
            THashMap<EHostType, TRegexpClassifierPtr>(descriptions.begin(), descriptions.end()));
    }

    TRequestClassifier TRequestClassifier::CreateFromJsonConfig(const TString& jsonConfFilePath) {
        TFileInput file(jsonConfFilePath);
        NJson::TJsonValue jsonValue;
        ReadJsonTree(file.ReadAll(), &jsonValue, true);
        std::array<TVector<TQueryRegexpHolder>, HOST_NUMTYPES> rules;
        for (const auto& jsonServiceValue : jsonValue.GetArraySafe()) {
            EHostType service = FromString(jsonServiceValue["service"].GetString());
            TJsonConfig::ParseRequestRegExps(jsonServiceValue["re_queries"], &rules[service]);
        }
        return CreateFromArray(rules);
    }

    TRequestClassifier TRequestClassifier::CreateFromArray(const std::array<TVector<TQueryRegexpHolder>, HOST_NUMTYPES>& regQueries)
    {
        TVector<std::pair<EHostType, TRequestClassifier::TRegexpClassifierPtr>> descriptions;
        descriptions.reserve(HOST_NUMTYPES);

        for (size_t hostTypeIdx = HOST_TYPE_FIRST; hostTypeIdx < HOST_TYPE_LAST; ++hostTypeIdx) {
            EHostType hostType = static_cast<EHostType>(hostTypeIdx);
            descriptions.emplace_back(hostType, TRegexpClassifierBuilder::CreateFromArray(regQueries[hostType], REQ_OTHER));
        }

        return TRequestClassifier::Create(
            THashMap<EHostType, TRegexpClassifierPtr>(descriptions.begin(), descriptions.end()));
    }

    TRegexpClassifierBuilder::TRegexpClassifierPtr
    TRegexpClassifierBuilder::CreateFromArray(const TVector<TQueryRegexpHolder>& regQueries,
                                              EReqType defaultReqType)
    {
        TVector<TRegexpClassifier::TClassDescription> rules;
        rules.reserve(regQueries.size());
        for (const auto& x : regQueries) {
            EReqType reqType = x.ReqType;
            if (x.Host.Defined() && x.Cgi.Defined()) {
                rules.emplace_back(GetFullPattern(*x.Host, x.Doc, *x.Cgi), reqType);
            } else if (x.Cgi.Defined()) {
                rules.emplace_back(GetFullPatternWithCgi(x.Doc, *x.Cgi), reqType);
            } else if (x.Host.Defined()) {
                rules.emplace_back(GetFullPatternWithHost(*x.Host, x.Doc), reqType);    
            } else {
                rules.emplace_back(GetFullPattern(x.Doc), reqType);
            }
        }
        return MakeSimpleShared<TRegexpClassifier>(rules.begin(), rules.end(), defaultReqType);
    }

    EReqType TRequestClassifier::DetectRequestType(EHostType hostType, TStringBuf doc, TStringBuf cgi,
                                                   EReqType defaultReqType) const {
        const auto& it = ClassifiersMap.find(hostType);
        if (it == ClassifiersMap.end()) {
            return defaultReqType;
        }

        auto fullUrl = TString{doc} + "?" + TString{cgi};
        return (*it->second)[fullUrl];
    }

    bool TRequestClassifier::IsMobileRequest(const TStringBuf& host, const TString& uri) const {
        if (host.StartsWith("m.")) {
            return true;
        }

        TStringBuf req = uri;
        TStringBuf doc;
        TStringBuf cgi;
        SplitUri(req, doc, cgi);

        return DetectRequestType(HostToHostType(uri), doc, cgi, REQ_OTHER) == REQ_MSEARCH;
    }

    TRequestClassifier TRequestClassifier::Create(const THashMap<EHostType, TRegexpClassifierPtr>& classifiersMap) {
        TRequestClassifier classifier;
        classifier.ClassifiersMap = classifiersMap;
        return classifier;
    }
}
