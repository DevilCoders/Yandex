#include "exception.h"

#include <kernel/common_server/library/report_builder/simple.h>

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/http/misc/httpreqdata.h>

void MakeErrorPage(IReplyContext::TPtr context, const ui32 code, const TString& error) try {
    if (!context) {
        return;
    }
    TRTYSimpleProtoReportBuilder builder(*context);
    BuildErrorPage(builder, code, error);
} catch (...) {
    DEBUG_LOG << "cannot MakeErrorPage for" << error << ": " << CurrentExceptionMessage() << Endl;
}
