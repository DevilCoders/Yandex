#pragma once

#include "json_config.h"
#include "req_types.h"

#include <library/cpp/regex/regexp_classifier/regexp_classifier.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <array>
#include <utility>


namespace NAntiRobot {


class TRequest;


class TRequestGroupClassifier {
public:
    TRequestGroupClassifier();

    explicit TRequestGroupClassifier(
        const std::array<TVector<TReqGroupRegExp>, HOST_NUMTYPES>& description
    );

    size_t NumGroups(EHostType service) const {
        return ServiceToClassifierData[service].GroupToName.size();
    }

    std::array<size_t, HOST_NUMTYPES> GetServiceToNumGroup() const {
        std::array<size_t, HOST_NUMTYPES> serviceToNumGroup;

        for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
            serviceToNumGroup[i] = NumGroups(static_cast<EHostType>(i));
        }

        return serviceToNumGroup;
    }

    EReqGroup Group(const TRequest& req) const;
    EReqGroup Group(EHostType service, TStringBuf url, TStringBuf header) const;
    const TString& GroupName(const TRequest& req) const;
    const TString& GroupName(EHostType service, EReqGroup group) const;

    static TRequestGroupClassifier CreateFromJsonConfig(const TString& jsonConfFilePath);

private:
    struct TClassifierData {
        TRegexpClassifier<EReqGroup> Classifier;
        TVector<TString> GroupToName;
    };

    TVector<TClassifierData> ServiceToClassifierData;
};


}
