#pragma once

#include <library/cpp/tvmauth/type.h>

#include <util/generic/string.h>

#include <time.h>

namespace NTvmknife {
    class TSimpleTvmClient {
    public:
        using TTvmId = NTvmAuth::TTvmId;

        struct TSshArgs {
            TTvmId Src = 0;
            TTvmId Dst = 0;
            TString Login;
            bool IsCacheEnabled = true;
        };

        static TString GetServiceTicketWithSshAgent(const TSshArgs& args);
        static TString GetServiceTicketWithSshPrivateKey(const TSshArgs& args,
                                                         const TString& privateKeyPath);

    protected:
        template <typename Func>
        static TString GetServiceTicketImpl(const TSshArgs& args,
                                            Func genSigns);

        template <typename Func>
        static TString SshkeyImpl(const TTvmId src,
                                  const TTvmId dst,
                                  const TString& login,
                                  Func genSigns,
                                  time_t now = time(nullptr));

        static TString GetTicketFromResponse(const TString& response, const TTvmId dst);
        static TString PrettifyJson(const TString& text);
    };
}
