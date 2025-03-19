#include "rating.h"

#include <kernel/snippets/config/config.h>

#include <kernel/snippets/custom/schemaorg_viewer/schemaorg_viewer.h>

#include <kernel/snippets/replace/replace.h>

#include <kernel/snippets/schemaorg/rating.h>

#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/tsnip.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>
#include <util/string/subst.h>

namespace NSnippets {

static bool IsValueBetween(const TUtf16String& ratingValue, double minValue, double maxValue) {
    double num = 0.0;
    return TryFromString(WideToUTF8(ratingValue), num) && num >= minValue && num <= maxValue;
}

static void FixDecimalPoint(TUtf16String& ratingValue) {
    SubstGlobal(ratingValue, wchar16(','), wchar16('.'));
}

bool TSchemaOrgRatingReplacer::HasRating(const TConfig& cfg, const TSchemaOrgArchiveViewer& viewer) {
    if (cfg.ExpFlagOff("ratings_info")) {
        return false;
    }
    if (cfg.IsMainPage()) {
        return false;
    }
    const NSchemaOrg::TRating* rating = viewer.GetRating();
    if (!rating) {
        return false;
    }
    if (!rating->GetRatingValue()) {
        return false;
    }
    return true;
}

void TSchemaOrgRatingReplacer::DoWork(TReplaceManager* manager) {
    const TReplaceContext& ctx = manager->GetContext();
    if (!HasRating(ctx.Cfg, ArcViewer)) {
        return;
    }
    const NSchemaOrg::TRating* rating = ArcViewer.GetRating();
    Y_ASSERT(rating);
    TUtf16String ratingValue(rating->GetRatingValue());
    TUtf16String bestRating(rating->GetBestRating());
    TUtf16String worstRating(rating->GetWorstRating());
    TUtf16String ratingCount(rating->GetRatingCount());

    FixDecimalPoint(ratingValue);
    FixDecimalPoint(bestRating);
    FixDecimalPoint(worstRating);

    // SNIPPETS-3735
    if (!bestRating && IsValueBetween(ratingValue, 0.0, 5.0)) {
        bestRating = u"5";
    }

    NJson::TJsonValue features(NJson::JSON_MAP);
    features["type"] = "snip_rating";
    features["ratingValue"] = WideToUTF8(ratingValue);
    features["ratingCount"] = WideToUTF8(ratingCount);
    features["bestRating"] = WideToUTF8(bestRating);
    features["worstRating"] = WideToUTF8(worstRating);

    NJson::TJsonValue data(NJson::JSON_MAP);
    data["content_plugin"] = true;
    data["type"] = "snip_rating";
    data["block_type"] = "construct";
    data["features"] = features;

    if (ctx.Cfg.ExpFlagOn("ratings_top3")) {
        NJson::TJsonValue allowedPositions(NJson::JSON_ARRAY);
        allowedPositions.AppendValue(0);
        allowedPositions.AppendValue(1);
        allowedPositions.AppendValue(2);
        data["allowed_positions"] = allowedPositions;
    }

    manager->GetExtraSnipAttrs().AddClickLikeSnipJson("snip_rating", NJson::WriteJson(data));
}

}
