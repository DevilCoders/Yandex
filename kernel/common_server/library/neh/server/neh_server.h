#pragma once

#include <kernel/common_server/util/events_rate_calcer.h>

#include <library/cpp/neh/multi.h>
#include <library/cpp/neh/neh.h>
#include <library/cpp/http/server/options.h>

#include <util/string/cast.h>

struct IObjectInQueue;

namespace NUtil {

    class TAbstractNehServer: public NNeh::IService {
    public:
        struct TOptions: public THttpServerOptions {
            TVector<TString> Schemes;

            TOptions() {}
            TOptions(const THttpServerOptions& options, const TString& scheme)
                : THttpServerOptions(options)
            {
                Schemes.push_back(scheme);
            }

            TOptions(const THttpServerOptions& options, const char* scheme)
                : THttpServerOptions(options)
            {
                Schemes.push_back(scheme);
            }

            template <class TCont>
            TOptions(const THttpServerOptions& options, const TCont& schemes)
                : THttpServerOptions(options)
                , Schemes(schemes.begin(), schemes.end())
            {}

            TString GetServerName() const {
                return Host + ':' + ToString(Port);
            }
        };
    protected:
        const TOptions Config;
        const NNeh::IServicesRef Requester;
        TAtomic RequestCounter;
        bool Started;
        TEventRate<> NehServerRequestRate;

    protected:
        virtual THolder<IObjectInQueue> DoCreateClientRequest(ui64 id, NNeh::IRequestRef req) = 0;

    public:
        TAbstractNehServer(const TOptions& config);
        virtual ~TAbstractNehServer() override;

        virtual void Start();
        virtual void Stop();
        virtual void ServeRequest(const NNeh::IRequestRef& req) override;

        inline NNeh::IServices& GetRequester() {
            return *Requester;
        }

    private:
        virtual THolder<IObjectInQueue> CreateClientRequest(ui64 id, NNeh::IRequestRef req) final {
            NehServerRequestRate.Hit();
            return DoCreateClientRequest(id, req);
        }
    };

}
