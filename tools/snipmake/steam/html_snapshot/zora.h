#pragma once

#include "fetch.h"

#include <yweb/robot/logel/logelutil.h>
#include <zora/client/lib/zoraclient.h>

#include <util/datetime/base.h>
#include <library/cpp/threading/future/legacy_future.h>

namespace NSteam
{
    using namespace NZoraClient;

    class TZoraFetcher: public TBaseFetcher, public IQueryMaker, public ILogger
    {
    private:
        THolder<TZoraClient> ZoraCl;
        THolder<NThreading::TLegacyFuture<> > Runner;
        TString Source;
        TDuration Timeout;
        int TimeoutSeconds;
        NFetcherMsg::ERequestType ReqType;

    public:
        TZoraFetcher(TDuration timeout, const TString& source, bool userproxy, bool ipv4);

        const char* GetName() const override
        {
            return "ZoraCl";
        }

        TDuration GetTimeout() const override
        {
            return Timeout;
        }

        TBusFetcherRequest* GetNextRequest() override;

        void OutputDocument(const TBusFetcherRequest *, const char *contentAndHeaders, size_t len, const char *urlAndInfo) override;
        void OutputLogel(const TBusFetcherRequest *, const TLogel<TUnknownRec> *logel) override;

        void OnBadResponse(TBusFetcherRequest *request, TBusFetcherResponse *response) override;

        void Terminate();
    };
}

