#include <library/cpp/monlib/push_client/auth.h>

#include <library/cpp/tvmauth/client/facade.h>

#include <util/string/builder.h>


namespace NSolomon {
namespace {
    struct TTvmProvider: IAuthProvider {
        TTvmProvider(const NTvmAuth::TTvmClient& client, NTvmAuth::TTvmId solomonId)
            : Client_{client}
            , SolomonId_{solomonId}
        {
        }

        void AddCredentials(THashMap<TString, TString>& headers) const override {
            headers["X-Ya-Service-Ticket"] = Client_.GetServiceTicketFor(SolomonId_);
        }

    private:
        const NTvmAuth::TTvmClient& Client_;
        NTvmAuth::TTvmId SolomonId_;
    };
} // namespace
    IAuthProviderPtr CreateTvmProvider(const NTvmAuth::TTvmClient& client, NTvmAuth::TTvmId solomonId) {
        return new TTvmProvider{client, solomonId};
    }
} // namespace NSolomon
