#pragma once
#include "helper.h"

class TFixedResponseServer: public NUtil::TAbstractNehServer {
public:
    struct TServerDeleter {
        static void Destroy(TFixedResponseServer* server);
    };
    struct TResponse {
        ui16 Code;
        TString Reply;
    };
    using TPtr = THolder<TFixedResponseServer, TServerDeleter>;
private:
    TVector<TResponse> Responses;
    size_t CallsCount = 0;
public:
    TFixedResponseServer(const TOptions& options,
                         const TVector<TResponse>& responses);
    static TPtr BuildAndRun(ui16 port, const TVector<TResponse>& responses);
    size_t GetCallsCount() const;

private:
    class TCallbackReply: public IObjectInQueue {
    public:
        TCallbackReply(NNeh::IRequestRef req, const TResponse& response);
        void Process(void* threadSpecificResource) override;

    protected:
        NNeh::IRequestRef ReqRef;
        TResponse Response;
    };

private:
    virtual THolder<IObjectInQueue> DoCreateClientRequest(ui64 /*id*/,
                                                           NNeh::IRequestRef req) override {
        return MakeHolder<TCallbackReply>(req, Responses[(CallsCount++) % Responses.size()]);
    }
};
