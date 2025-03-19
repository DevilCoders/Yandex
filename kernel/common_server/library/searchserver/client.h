#pragma once
#include "simple/client.h"
#include "simple/replier.h"
#include <search/request/data/reqdata.h>

class TSearchHttpReplyContext: public IRDReplyContext<TCommonHttpReplyContext<TSearchRequestData>> {
private:
    using TBase = IRDReplyContext<TCommonHttpReplyContext<TSearchRequestData>>;
    using TBase::Client;
public:
    using TRequestData = TSearchRequestData;
    using TBase::TBase;

    virtual TRequestData& MutableRequestData() override {
        return Client->MutableRequestData();
    }

    virtual const TRequestData& GetRequestData() const override {
        return Client->GetRequestData();
    }
};

using TSearchClient = THttpClientImpl<TSearchHttpReplyContext>;
