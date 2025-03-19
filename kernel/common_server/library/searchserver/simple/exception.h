#pragma once

#include "replier.h"

#include <kernel/search_daemon_iface/reqtypes.h>
#include <kernel/reqerror/reqerror.h>

#include <library/cpp/http/misc/httpcodes.h>

#include <util/generic/yexception.h>

class TCommonSearch;
class TSearchRequestData;

class TSearchException: public yexception {
private:
    const ui32 HttpCode;
    const yxErrorCode YxCode;

public:
    TSearchException(const ui32 httpCode, const yxErrorCode errorCode = yxUNKNOWN_ERROR)
        : HttpCode(httpCode)
        , YxCode(errorCode)
    {}

    ui32 GetHttpCode() const {
        return HttpCode;
    }

    yxErrorCode GetErrorCode() const {
        return YxCode;
    }
};

template <class T>
void BuildErrorPage(T& builder, ui32 code, const TString& error) try {
    builder.AddErrorMessage(error);
    builder.MarkIncomplete();
    builder.Finish(code);
} catch (...) {
    DEBUG_LOG << "cannot BuildErrorPage for" << error << ": " << CurrentExceptionMessage() << Endl;
}

void MakeErrorPage(IReplyContext::TPtr context, const ui32 code, const TString& error);
