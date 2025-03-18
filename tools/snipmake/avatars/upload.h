#pragma once

#include <library/cpp/neh/neh.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/string.h>
#include <util/generic/ptr.h>

namespace NMdsAvatars {
    static const TString AVATARS_INTERNAL_URL = "http://avatars-int.mds.yandex.net:13000/";
    static const TString AVATARS_INTERNAL_TESTING_URL = "http://avatars-int.mdst.yandex.net:13000/";

    bool CreateUploadMessage(
        NNeh::TMessage& msg,
        bool testing,
        const TString& nspace,
        const TString& id,
        const TString& blob,
        bool svg = false,
        i32 ttlInDays = -1);

    void ParseUploadResult(
        TString& error,
        NSc::TValue& parsed,
        const NNeh::TResponseRef& response);
}
