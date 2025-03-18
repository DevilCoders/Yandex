#pragma once

#include <library/cpp/http/server/http.h>

struct TIpAddress;

namespace NHttp {
    struct TRequest {
        const TIpAddress* Local;
        const TIpAddress* Remote;
        THttpInput* In;
        THttpOutput* Out;
    };

    class IRequester {
    public:
        virtual void Reply(const TRequest& req) = 0;
    };

    template <class T>
    class TRequester: public IRequester {
    public:
        inline TRequester(T* t)
            : T_(t)
        {
        }

        void Reply(const TRequest& req) override {
            T_->Reply(req);
        }

    private:
        T* T_;
    };

    void ServeHttpImpl(IRequester* r, const THttpServerOptions& options);
    void ServeBasicHttpImpl(IRequester* r, const THttpServerOptions& options);
    void ServeUnqueuedHttpImpl(IRequester* r, const THttpServerOptions& options);
}

template <class T>
static inline void ServeHttp(T* t, const THttpServerOptions& options) {
    using namespace NHttp;

    TRequester<T> r(t);

    ServeHttpImpl(&r, options);
}

template <class T>
static inline void ServeBasicHttp(T* t, const THttpServerOptions& options) {
    using namespace NHttp;

    TRequester<T> r(t);

    ServeBasicHttpImpl(&r, options);
}

template <class T>
static inline void ServeUnqueuedHttp(T* t, const THttpServerOptions& options) {
    using namespace NHttp;

    TRequester<T> r(t);

    ServeUnqueuedHttpImpl(&r, options);
}
