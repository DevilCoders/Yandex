#pragma once

#include "json_config.h"
#include "req_types.h"

#include <library/cpp/regex/regexp_classifier/regexp_classifier.h>

namespace NAntiRobot {
    class TRegexpClassifierBuilder {
    private:
        using TRegexpClassifier = TRegexpClassifier<EReqType>;
        using TRegexpClassifierPtr = TSimpleSharedPtr<const TRegexpClassifier>;

    public:
        static TRegexpClassifierPtr CreateFromFile(const TString& filename, EReqType defaultReqType);
        static TRegexpClassifierPtr CreateFromLines(const TVector<TString>& lines, EReqType defaultReqType);
        static TRegexpClassifierPtr CreateFromArray(const TVector<TQueryRegexpHolder>& regQueries,
                                                    EReqType defaultReqType);
    private:
        static TString GetFullPattern(const TString& doc);
        static TString GetFullPatternWithCgi(const TString& doc, const TString& cgi);
        static TString GetFullPatternWithHost(const TString& host, const TString& doc);
        static TString GetFullPattern(const TString& host, const TString& doc, const TString& cgi);
        static TVector<TString> ReadAllRules(const TString& filename);
    };

    class TRequestClassifier {
    private:
        using TRegexpClassifierPtr = TSimpleSharedPtr<const TRegexpClassifier<EReqType>>;

        THashMap<EHostType, TRegexpClassifierPtr> ClassifiersMap;
    public:
        static TRequestClassifier CreateFromRules(const TString& rulesPath);
        static TRequestClassifier CreateFromArray(const std::array<TVector<TQueryRegexpHolder>, HOST_NUMTYPES>& regQueries);
        static TRequestClassifier CreateFromJsonConfig(const TString& jsonConfFilePath);

        EReqType DetectRequestType(EHostType hostType, TStringBuf doc, TStringBuf cgi, EReqType defaultReqType) const;
        bool IsMobileRequest(const TStringBuf& host, const TString& uri) const;

    private:
        static TRequestClassifier Create(const THashMap<EHostType, TRegexpClassifierPtr>& classifiersMap);
    };
}
