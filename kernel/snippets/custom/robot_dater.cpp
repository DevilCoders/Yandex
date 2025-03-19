#include "robot_dater.h"

#include <kernel/snippets/archive/doc_lang.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/sent_match/extra_attrs.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/langs/langs.h>

namespace NSnippets {

TRobotDateReplacer::TRobotDateReplacer()
    : IReplacer("robot_dater")
{
}

void TRobotDateReplacer::DoWork(TReplaceManager* manager) {
    const TReplaceContext& ctx = manager->GetContext();

    NJson::TJsonValue features(NJson::JSON_MAP);
    features["type"] = "robot_date";
    TDocInfos::const_iterator robotDate = ctx.DocInfos.find("RobotDate");
    TDocInfos::const_iterator robotDateScore = ctx.DocInfos.find("RobotDateScore");
    if (robotDate == ctx.DocInfos.end() || robotDateScore == ctx.DocInfos.end()){
        return;
    }

    features["RobotDate"] = robotDate->second;
    features["RobotDateScore"] = robotDateScore->second;

    NJson::TJsonValue data(NJson::JSON_MAP);
    data["block_type"] = "construct";
    data["content_plugin"] = true;
    data["features"] = features;
    manager->GetExtraSnipAttrs().AddClickLikeSnipJson("robot_date", NJson::WriteJson(data));
}

} // namespace NSnippets
