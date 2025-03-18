#include "request_group_classifier.h"

#include "request_params.h"

#include <util/generic/yexception.h>


namespace NAntiRobot {

static const TString GENERIC_GROUP_NAME = "generic";

TRequestGroupClassifier::TRequestGroupClassifier() {
    ServiceToClassifierData.reserve(HOST_NUMTYPES);

    for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
        ServiceToClassifierData.push_back(TClassifierData{
            TRegexpClassifier<EReqGroup>({}, EReqGroup::Generic),
            {GENERIC_GROUP_NAME}
        });
    }
}

TRequestGroupClassifier TRequestGroupClassifier::CreateFromJsonConfig(const TString& jsonConfFilePath) {
    TFileInput file(jsonConfFilePath);
    NJson::TJsonValue jsonValue;
    ReadJsonTree(file.ReadAll(), &jsonValue, true);
    std::array<TVector<TReqGroupRegExp>, HOST_NUMTYPES> rules;
    for (const auto& jsonServiceValue : jsonValue.GetArraySafe()) {
        EHostType service = FromString(jsonServiceValue["service"].GetString());
        TJsonConfig::ParseRequestGroupRegExps(jsonServiceValue["re_groups"], &rules[service]);
    }
    return TRequestGroupClassifier(rules);
}

TRequestGroupClassifier::TRequestGroupClassifier(
    const std::array<TVector<TReqGroupRegExp>, HOST_NUMTYPES>& description
) {
    ServiceToClassifierData.reserve(description.size());

    for (const auto& entry : description) {
        TVector<TRegexpClassifier<EReqGroup>::TClassDescription> classes;
        classes.reserve(entry.size());

        THashMap<TString, EReqGroup> groupNameToGroup = {{GENERIC_GROUP_NAME, EReqGroup::Generic}};
        TVector<TString> groupToName = {GENERIC_GROUP_NAME};

        for (const auto& entryItem : entry) {
            if (!groupNameToGroup.contains(entryItem.Group)) {
                const size_t newIdx = groupNameToGroup.size();
                groupNameToGroup[entryItem.Group] = static_cast<EReqGroup>(newIdx);
                Y_ASSERT(groupToName.size() == newIdx);
                groupToName.push_back(entryItem.Group);
            }

            const EReqGroup group = groupNameToGroup[entryItem.Group];

            const auto re = entryItem.Host.Defined()
                            ? *entryItem.Host : "[^/]+"
                            + (entryItem.Cgi.Defined()
                            ? entryItem.Doc + "\\?" + *entryItem.Cgi
                            : entryItem.Doc + "\\?.*");
            classes.emplace_back(re, group);
        }

        ServiceToClassifierData.push_back(TClassifierData{
            TRegexpClassifier(
                classes.begin(), classes.end(),
                EReqGroup::Generic
            ),
            std::move(groupToName)
        });
    }
}

EReqGroup TRequestGroupClassifier::Group(EHostType service, TStringBuf url, TStringBuf header) const {
    if (!header.empty()) {
        const size_t groupIdx = FindIndex(ServiceToClassifierData[service].GroupToName, header);
        if (groupIdx != NPOS) {
            return static_cast<EReqGroup>(groupIdx);
        }
    }
    return ServiceToClassifierData[service].Classifier[url];
}

EReqGroup TRequestGroupClassifier::Group(const TRequest& req) const {
    if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
        return EReqGroup::Generic;
    }

    const auto url = TString(req.Host) + req.Doc + "?" + req.Cgi;

    TStringBuf header = "";
    constexpr TStringBuf reqGroupHeaderName = "X-Antirobot-Req-Group";
    if (req.Headers.Has(reqGroupHeaderName)) {
        header = req.Headers.Get(reqGroupHeaderName);
    }
    return Group(req.HostType, url, header);
}

const TString& TRequestGroupClassifier::GroupName(const TRequest& req) const {
    return GroupName(req.HostType, req.ReqGroup);
}

const TString& TRequestGroupClassifier::GroupName(EHostType service, EReqGroup group) const {
    const auto& groupToName = ServiceToClassifierData[service].GroupToName;

    const size_t groupIdx = static_cast<size_t>(group);

    Y_ENSURE(
        groupIdx < groupToName.size(),
        "Group index out of bounds"
    );

    return groupToName[groupIdx];
}


} // namespace NAntiRobot
