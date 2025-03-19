#include "tvm.h"
#include <library/cpp/digest/md5/md5.h>

namespace NExternalAPI {

    TTVMRequestCustomization::TFactory::TRegistrator<TTVMRequestCustomization> TTVMRequestCustomization::Registrator(TTVMRequestCustomization::GetTypeName());

    void TTVMRequestCustomization::DoInit(const TYandexConfig::Section* section) {
        const auto& dir = section->GetDirectives();
        DestinationClientId = dir.Value<ui32>("DestinationClientId", DestinationClientId);
        if (!dir.GetNonEmptyValue("TvmClientName", TvmClientName)) {
            dir.GetNonEmptyValue("SelfClientId", TvmClientName);
        }

        dir.GetValue("TicketString", TicketString);
        dir.GetValue("Token", Token);
        if (!TicketString && !Token) {
            AssertCorrectConfig(!!TvmClientName, "TvmClientName required");
        }
    }

    void TTVMRequestCustomization::DoToString(IOutputStream& os) const {
        os << "DestinationClientId: " << DestinationClientId << Endl;
        os << "TvmClientName: " << TvmClientName << Endl;
        os << "TicketString: " << TicketString << Endl;
        os << "Token: " << MD5::Calc(Token) << Endl;
    }

    bool TTVMRequestCustomization::DoTuneRequest(const IServiceApiHttpRequest& /*baseRequest*/, NNeh::THttpRequest& request, const NExternalAPI::IRequestCustomizationContext* cContext, const NExternalAPI::TSender* /*sender*/) const {
        try {
            if (cContext) {
                TAtomicSharedPtr<NTvmAuth::TTvmClient> tvm;
                tvm = cContext->GetTvmManager()->GetTvmClient(TvmClientName);
                if (tvm && DestinationClientId) {
                    request.AddHeader("X-Ya-Service-Ticket", tvm->GetServiceTicketFor(DestinationClientId));
                }
            }

            if (TicketString) {
                request.AddHeader("X-Ya-Service-Ticket", TicketString);
            }

            if (Token) {
                request.AddHeader("Authorization", "Bearer " + Token);
            }
        } catch (...) {
            ERROR_LOG << CurrentExceptionMessage() << Endl;
            request.AddHeader("X-Ya-Service-Ticket", "");
        }
        return true;
    }

}
