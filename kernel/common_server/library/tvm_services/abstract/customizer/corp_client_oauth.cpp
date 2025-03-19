#include "corp_client_oauth.h"
#include <kernel/common_server/library/logging/events.h>

namespace NExternalAPI {

    TCorpClientOAuthRequestCustomization::TFactory::TRegistrator<TCorpClientOAuthRequestCustomization> TCorpClientOAuthRequestCustomization::Registrator(TCorpClientOAuthRequestCustomization::GetTypeName());

    void TCorpClientOAuthRequestCustomization::DoInit(const TYandexConfig::Section* section) {
        CHECK_WITH_LOG(section);
        DefaultToken = section->GetDirectives().Value<TString>("DefaultToken", DefaultToken);

        auto allChildren = section->GetAllChildren();
        auto tokens = allChildren.find("Tokens");
        CHECK_WITH_LOG(tokens != allChildren.end());
        const auto tokensSection = tokens->second;
        for (auto&& [corpClientId, _] : tokensSection->GetDirectives()) {
            const auto token = tokensSection->GetDirectives().Value<TString>(corpClientId, TString());
            CorpClientToken.emplace(corpClientId, token);
        }
    }

    void TCorpClientOAuthRequestCustomization::DoToString(IOutputStream& os) const {
        os << "DefaultToken: " << MaskToken(DefaultToken) << Endl;
        os << "<Tokens>" << Endl;
        for (auto&& [corpClient, token] : CorpClientToken) {
            os << corpClient << ":" << MaskToken(token) << Endl;
        }
        os << "</Tokens>" << Endl;
    }

    TString TCorpClientOAuthRequestCustomization::MaskToken(const TString& token) const {
        if (token.size() < 10) {
            return "(" + ::ToString(token.size()) + " chars)";
        }
        return token.substr(0, 2) + "..." + token.substr(token.size() - 2, 2);
    }

    bool TCorpClientOAuthRequestCustomization::DoTuneRequest(const IServiceApiHttpRequest& /*baseRequest*/, NNeh::THttpRequest& request, const NExternalAPI::IRequestCustomizationContext*, const NExternalAPI::TSender* /*sender*/) const {
        TString clientId;
        request.AddHeader("Content-Type", "application/json");

        const auto clientIt = request.GetHeaders().find("X-B2B-Client-Id");
        if (clientIt != request.GetHeaders().end()) {
            clientId = clientIt->second;
        }

        {
            const auto it = CorpClientToken.find(clientId);
            if (it != CorpClientToken.end()) {
                request.AddHeader("Authorization", "Bearer " + it->second);
                TFLEventLog::Log("customization")("add_auth_header", MaskToken(it->second));
            } else {
                request.AddHeader("Authorization", "Bearer " + DefaultToken);
                TFLEventLog::Log("customization")("add_default_auth_header", MaskToken(DefaultToken));
            }
        }

        return true;
    }

    TVector<TString> TCorpClientOAuthRequestCustomization::GetCorpClientIds() const {
        TVector<TString> result;
        result.emplace_back("");
        for (auto&& [corpClientId, _] : CorpClientToken) {
            result.emplace_back(corpClientId);
        }
        return result;
    }

}
