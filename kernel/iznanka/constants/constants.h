#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/types.h>

namespace NIznanka {
    namespace NConstants {
        const static TString SP_REARR_APPLY_BLENDER = "ApplyBlender";
        const static TString SP_REARR_ORG_WIZARD = "OrgWizard";
        const static TString SP_REARR_RECOMMENDER_IZNANKA = "RecommenderIznanka";

        const static TString SP_FACTORS = "factors";
        const static TString SP_FMLS = "fmls";
        const static TString SP_DYNAMIC_FACTORS = "dynamic_factors";

        const static TString SP_ARTICLENESS = "Articleness";
        const static TString SP_RELEVANCE = "Relevance";
        const static TString SP_RESULT = "Result";
        const static TString SP_SOURCE = "Source";
        const static TString SP_SOURCE_ID_PAIRS = "SourceIdPairs";

        const static TString SEARCHPROP_PREFIX = "UPPER";

        const static TString PATH_NOTHING = "/iznanka/nothing";
        const static TString PATH_LINE = "/iznanka/line";
        const static TString PATH_TEASER = "/iznanka/teaser";
        const static TString PATH_OPENED = "/iznanka/opened";
        const static TString PATH_MAXIMIZED = "/iznanka/maximized";

        const static TString POOL_FIELD_CLICKS = "clicks";
        const static TString POOL_FIELD_NAVIGS = "navigs";
        const static TString POOL_FIELD_TESTIDS = "testids";
        const static TString POOL_FIELD_BUCKETS = "buckets";
        const static TString POOL_FIELD_BROWSER_EXP = "browser_exp";
        const static TString POOL_FIELD_TIMESTAMP = "timestamp";
        const static TString POOL_FIELD_UUID = "uuid";
        const static TString POOL_FIELD_DEVICEID = "deviceid";
        const static TString POOL_FIELD_YANDEXUID = "yandexuid";
        const static TString POOL_FIELD_USER_REGION = "user_region";
        const static TString POOL_FIELD_REQID = "reqid";
        const static TString POOL_FIELD_FULL_REQUEST = "full_request";
        const static TString POOL_FIELD_URL = "url";
        const static TString POOL_FIELD_PAGE_URL = "page_url";
        const static TString POOL_FIELD_FRAME_URL = "frame_url";
        const static TString POOL_FIELD_TEXT = "text";
        const static TString POOL_FIELD_TAB_ID = "tab_id";
        const static TString POOL_FIELD_RESPONSE_TYPE = "response_type";

        const static TString POOL_SUBFIELD_PATH = "path";
        const static TString POOL_SUBFIELD_PATH_SOURCE = "path_source";
        const static TString POOL_SUBFIELD_PATH_DESTINATION = "path_destination";
        const static TString POOL_SUBFIELD_ACTION = "action";
        const static TString POOL_SUBFIELD_DWELLTIME = "dwelltime";
        const static TString POOL_SUBFIELD_DWELLTIME_ON_SERVICE = "dwelltime_on_service";
        const static TString POOL_SUBFIELD_TIMESTAMP = "timestamp";
        const static TString POOL_SUBFIELD_IS_DYNAMIC = "is_dynamic";

        TString DotSearchProps(const TVector<TString>& searchProps);
        TString UnderscoreSearchProps(const TVector<TString>& searchProps);
    }

}
