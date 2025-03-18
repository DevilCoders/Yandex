#include "upload.h"

#include <library/cpp/neh/neh.h>
#include <library/cpp/neh/http_common.h>
#include <library/cpp/scheme/scheme.h>

#include <util/stream/str.h>
#include <util/string/builder.h>

namespace NMdsAvatars {
    TString BuildUploadFormBody(
        const TString& data,
        const TString& boundary,
        const TString& filename)
    {
        TStringBuilder result;

        result
            << "--" << boundary << "\r\n"
            << "Content-Disposition: form-data; "
            << "name=\"" << filename << "\"; "
            << "filename=\""<< filename << "\"" << "\r\n"
            << "Content-Length: " << data.size() << "\r\n"
            << "\r\n"
            << data << "\r\n"
            << "--" << boundary
            << "--\r\n";

        return result;
    }

    bool CreateUploadMessage(
        NNeh::TMessage& msg,
        bool testing,
        const TString& nspace,
        const TString& id,
        const TString& blob,
        bool svg,
        i32 ttlInDays)
    {
        const TString boundary = "dfgfsdfdhwyewcncnxw";

        TStringBuilder uploadUrl;

        if (testing) {
            uploadUrl << AVATARS_INTERNAL_TESTING_URL;
        } else {
            uploadUrl << AVATARS_INTERNAL_URL;
        }

        uploadUrl << "put-" << nspace << "/" << id;

        if (ttlInDays > 0) {
            uploadUrl << "?expire=" << ttlInDays << "d";
        }

        msg = NNeh::TMessage::FromString(uploadUrl);

        return NNeh::NHttp::MakeFullRequest(
            msg,
            "Accept: application/json",
            BuildUploadFormBody(blob, boundary, svg ? "svg" : "file"),
            "multipart/form-data; boundary=" + boundary,
            NNeh::NHttp::ERequestType::Post);
    }

    void ParseUploadResult(
        TString& error,
        NSc::TValue& parsed,
        const NNeh::TResponseRef& response)
    {
        error.clear();
        parsed.Clear();

        if (!response) {
            error = "No response";
            return;
        }

        NSc::TValue::FromJson(parsed, response->Data);

        if (response->IsError()) {
            error = response->GetErrorText();
            return;
        }
    }
}
