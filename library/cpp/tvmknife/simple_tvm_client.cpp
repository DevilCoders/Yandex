#include "simple_tvm_client.h"

#include "cache.h"
#include "output.h"

#include <library/cpp/http/simple/http_client.h>
#include <library/cpp/json/json_prettifier.h>
#include <library/cpp/json/fast_sax/parser.h>
#include <library/cpp/ssh_sign/sign.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/quote/quote.h>

#include <util/string/strip.h>

namespace NTvmknife {
    static TString Base64EncodeUrlValid(TStringBuf s) {
        TString result = Base64EncodeUrl(s);
        result = StripStringRight(result, EqualsStripAdapter(','));
        return result;
    }

    TString TSimpleTvmClient::GetServiceTicketWithSshAgent(const TSshArgs& args) {
        return GetServiceTicketImpl(
            args,
            [&]() {
                return NSS::SignByAny(args.Login);
            });
    }

    TString TSimpleTvmClient::GetServiceTicketWithSshPrivateKey(const TSshArgs& args,
                                                                const TString& privateKeyPath) {
        return GetServiceTicketImpl(
            args,
            [&]() {
                std::vector<NSS::TLazySignerPtr> res;
                res.push_back(NSS::SignByRsaKey(privateKeyPath));
                return res;
            });
    }

    template <typename Func>
    TString TSimpleTvmClient::GetServiceTicketImpl(const TSshArgs& args,
                                                   Func genSigns) {
        TString res;
        if (args.IsCacheEnabled) {
            res = TCache::Get().GetSrvTicket(args.Src, args.Dst, args.Login);
            if (res) {
                return res;
            }
        }

        res = GetTicketFromResponse(
            SshkeyImpl(args.Src, args.Dst, args.Login, genSigns),
            args.Dst);

        if (args.IsCacheEnabled) {
            TCache::Get().SetSrvTicket(res, args.Src, args.Dst, args.Login);
        }
        return res;
    }

    template <typename Func>
    TString TSimpleTvmClient::SshkeyImpl(const TTvmId srcNum,
                                         const TTvmId dstNum,
                                         const TString& login,
                                         Func genSigns,
                                         time_t now) {
        Y_ENSURE(login, "login cannot be empty");

        const TString ts = IntToString<10>(now);
        const TString src = IntToString<10>(srcNum);
        const TString dst = IntToString<10>(dstNum);

        TString body;
        body.append("grant_type=sshkey");
        body.append("&ts=").append(ts);
        body.append("&src=").append(src);
        body.append("&dst=").append(dst);
        body.append("&login=").append(CGIEscapeRet(login));
        body.append("&tvmknife_");
        body.append("&ssh_sign=");

        const TString toSign = ts + "|" + src + "|" + dst;

        TSimpleHttpClient http("https://tvm-api.yandex.net", 443);

        for (NSS::TLazySignerPtr& signer : genSigns()) {
            std::optional<NSS::TResult> res;

            while ((res = signer->SignNext(toSign))) {
                const NSS::TResult& result = *res;
                if (const auto* err = std::get_if<NSS::TErrorMsg>(&result)) {
                    ::NTvmknife::NOutput::Out() << "Failed to sign request:" << *err << Endl;
                    continue;
                }

                const NSS::TSignedData& signedData = std::get<NSS::TSignedData>(result);
                TString post = body + Base64EncodeUrlValid(signedData.Sign);
                if (signedData.Type == NSS::ESignType::CERT) {
                    post += "&public_cert=" + Base64EncodeUrlValid(signedData.Key);
                }

                TString out;
                try {
                    TStringOutput s(out);
                    http.DoPost("/2/ticket", post, &s);
                    return out;
                } catch (const std::exception& e) {
                    ::NTvmknife::NOutput::Out() << "Failed to fetch ticket from TVM:" << Endl
                                                << e.what()
                                                << PrettifyJson(out) << Endl;
                }
            }
        }

        ythrow yexception() << "All attempts to get ticket are failed";
    }

    TString TSimpleTvmClient::GetTicketFromResponse(const TString& response, const TTvmId dst) {
        try {
            ::NJson::TJsonValue doc;
            Y_ENSURE(::NJson::ReadJsonTree(response, &doc));
            ::NJson::TJsonValue value;
            Y_ENSURE(doc.GetValue(ToString(dst), &value));
            Y_ENSURE(value.IsMap());

            Y_ENSURE(!value.Has("error"));

            ::NJson::TJsonValue ticket;
            Y_ENSURE(value.GetValue("ticket", &ticket));
            Y_ENSURE(ticket.IsString());
            return ticket.GetString();
        } catch (const std::exception& e) {
            ythrow yexception() << "Failed to get ticket from TVM response: " << e.what() << Endl
                                << response << Endl;
        }
    }

    TString TSimpleTvmClient::PrettifyJson(const TString& text) {
        return NJson::ValidateJson(text) ? NJson::PrettifyJson(text) : text;
    }
}
