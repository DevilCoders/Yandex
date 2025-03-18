#pragma once
#include "bad_request_handler.h"
#include "request_context.h"

#include <antirobot/lib/antirobot_response.h>

#include <library/cpp/http/cookies/lctable.h>

#include <util/generic/ptr.h>

#include <functional>

namespace NAntiRobot {

template<class TSelector>
class TSelectiveHandler {
private:
    using TFunc = std::function<NThreading::TFuture<TResponse>(TRequestContext&)>;

public:
    template<class TSelector_ = TSelector, class = std::enable_if_t<std::is_constructible<TSelector_>::value>>
    TSelectiveHandler()
        : Selector()
        , DefaultHandler(TNotFoundHandler())
    {
    }
    explicit TSelectiveHandler(TSelector selector)
        : Selector(selector)
        , DefaultHandler(TNotFoundHandler())
    {
    }

    TSelectiveHandler& Add(const TStringBuf& value, const TFunc& handler) {
        return AddImpl(value, handler);
    }

    TSelectiveHandler& Add(const TSelectiveHandler& that) {
        for (const auto& i : that.Handlers) {
            this->AddImpl(i.first, i.second);
        }
        return *this;
    }

    TSelectiveHandler& Default(const TFunc& handler)
    {
        DefaultHandler = handler;
        return *this;
    }

    NThreading::TFuture<TResponse> operator()(TRequestContext& rc) {
        const auto& handler = Handlers.Get(Selector(*rc.Req));
        return !!handler ? handler(rc)
                         : DefaultHandler(rc);
    }

private:
    TSelectiveHandler& AddImpl(const TStringBuf& value, const TFunc& handler) {
        Y_VERIFY_DEBUG(!Handlers.Has(value), "Value '%s' already exists", TString(value).c_str());
        Handlers.Add(value, handler);
        return *this;
    }

    TSelector Selector;

    TLowerCaseTable<TFunc> Handlers;
    TFunc DefaultHandler;
};

/*
 * Below are defined the selectors themselves.
 */

struct THttpMethodSelector {
    TStringBuf operator() (const TRequest& req) {
        return req.RequestMethod;
    }
};

struct TUrlLocationSelector {
    TStringBuf operator() (const TRequest& req) {
        return req.Doc;
    }
};

class TCgiSelector {
public:
    TCgiSelector(const TStringBuf& paramName)
        : ParamName(paramName)
    {
    }

    TStringBuf operator() (const TRequest& req) {
        return req.CgiParams.Get(ParamName);
    }

private:
    TStringBuf ParamName;
};

inline TSelectiveHandler<THttpMethodSelector> HandleHttpMethod() {
    return TSelectiveHandler<THttpMethodSelector>();
}

inline TSelectiveHandler<TUrlLocationSelector> HandleUrlLocation() {
    return TSelectiveHandler<TUrlLocationSelector>();
}

inline TSelectiveHandler<TCgiSelector> HandleCgi(const TStringBuf& cgiParamName) {
    return TSelectiveHandler<TCgiSelector>(TCgiSelector(cgiParamName));
}

}
