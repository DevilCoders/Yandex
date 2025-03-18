#pragma once

#include "page_template.h"
#include "resource_selector.h"
#include "request_context.h"
#include "request_params.h"

#include <antirobot/lib/antirobot_response.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/mime/types/mime.h>
#include <library/cpp/threading/future/future.h>

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAntiRobot {
    class TBlockedHandler {
    private:
        struct TContent {
            const MimeTypes ContentType;
            const TString Content;
        };
    public:
        TBlockedHandler();

        NThreading::TFuture<TResponse> operator()(TRequestContext& rc);

    private:
        TContent GenerateContent(const TRequestContext& rc) const;
        TContent JsonBlock(const TRequest& req) const;

    private:
        TResourceSelector<TPageTemplate> PageSelector;
        TString PartnerPage;
        TPageTemplate AjaxPage;
        TString MobileJsonSearchPage;
    };
}
