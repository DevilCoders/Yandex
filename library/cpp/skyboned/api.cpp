#include "api.h"

#include <library/cpp/http/client/client.h>
#include <library/cpp/http/client/request.h>
#include <library/cpp/json/json_value.h>

namespace {
    void DoApiRequest(const NSkyboneD::NApi::TApiConfig& config,
                      const TString& tvmHeader,
                      const TString& apiMethod,
                      const TString& rbtorrent,
                      const TString& requestData)
    {
        Y_ENSURE(config.MaxAttempts > 0);

        NHttp::TFetchOptions options;
        options.SetContentType("application/json");
        options.SetMethod("POST");
        options.SetTimeout(config.Timeout);
        options.SetPostData(requestData);

        TDuration currentFailureDelay = config.InitFailureDelay;
        size_t remainingAttempts = config.MaxAttempts;
        while (remainingAttempts--) {
            auto result = NHttp::Fetch(NHttp::TFetchQuery{config.Endpoint + apiMethod,
                                                          {tvmHeader},
                                                          options});
            if (result->Code == 200) {
                return;
            } else if (result->Code == 400 || result->Code == 403) {
                ythrow yexception() << "request for torrent " << rbtorrent << " is broken. "
                                    << "Code " << result->Code
                                    << ". Raw request " << requestData
                                    << ". Response " << result->Data;
            } else if (remainingAttempts) {
                Sleep(currentFailureDelay);
                currentFailureDelay *= 2;
            }
        }

        ythrow yexception() << "run out of attempts for torrent " << rbtorrent
                            << "method " << apiMethod
                            << " request " << requestData;
    }
}

namespace NSkyboneD::NApi {
    void RemoveResource(const TApiConfig& config,
                        const TString& tvmHeader,
                        const TString& rbtorrent,
                        const TString& sourceId)
    {
        const auto fixedPrefix = "rbtorrent:"sv;
        const int uidSize = 40;
        Y_ENSURE(rbtorrent.StartsWith(fixedPrefix));
        Y_ENSURE(rbtorrent.Size() == fixedPrefix.size() + uidSize);
        const TString uid = rbtorrent.substr(fixedPrefix.size(), 40);

        NJson::TJsonValue deleteRequestJson;
        deleteRequestJson["uid"] = uid;
        if (sourceId) {
            deleteRequestJson["source_id"] = sourceId;
        }

        DoApiRequest(config,
                     tvmHeader,
                     "/remove_resource",
                     rbtorrent,
                     deleteRequestJson.GetStringRobust());
    }


    TString AddResource(const TApiConfig& config,
                        const TString& tvmHeader,
                        const NJson::TJsonValue& request)
    {
        const TString rbtorrent = "rbtorrent:" + request["uid"].GetString();

        DoApiRequest(config,
                     tvmHeader,
                     "/add_resource",
                     rbtorrent,
                     request.GetStringRobust());

        return rbtorrent;
    }
}
