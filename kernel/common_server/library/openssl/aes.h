#pragma once

#include "oio.h"
#include "types.h"

#include <kernel/common_server/util/accessor.h>

namespace NOpenssl {
    bool AESEncrypt(const TString& key, const TString& input, TString& output);
    bool AESEncrypt(const TString& key, const TBuffer& input, TString& output);
    bool AESDecrypt(const TString& key, const TString& input, TString& output);

    bool AESGCMEncrypt(const TString& key, const TString& input, TString& output, const TString& keyId = "default");
    bool AESGCMDecrypt(const TString& key, const TString& input, TString& output);

    TString GenerateAESKey(ui32 len = 32);

    class TGcmToken {
        CS_ACCESS(TGcmToken, TString, KeyId, "default");
        CSA_DEFAULT(TGcmToken, TString, IV);
        CSA_DEFAULT(TGcmToken, TString, Tag);
        CSA_DEFAULT(TGcmToken, TString, Data);

    public:
        TString ToString() const;
        bool FromString(const TString& str);
    };
}
