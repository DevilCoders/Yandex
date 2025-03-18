#pragma once
#include "bad_request_handler.h"

namespace NAntiRobot {

struct TEnv;
class TRequest;

template<class TPredicate, class TTrueHandler = TNotFoundHandler, class TFalseHandler = TNotFoundHandler>
class TConditionalHandler {
public:
    explicit TConditionalHandler(TPredicate predicate,
                                 TTrueHandler trueHandler = TTrueHandler(),
                                 TFalseHandler falseHandler = TFalseHandler())
        : Predicate(predicate)
        , TrueHandler(trueHandler)
        , FalseHandler(falseHandler)
    {
    }

    NThreading::TFuture<TResponse> operator()(TRequestContext& rc) {
        return Predicate(rc) ? TrueHandler(rc)
                             : FalseHandler(rc);
    }

    template<class TNewTrueHandler>
    [[nodiscard]] TConditionalHandler<TPredicate, TNewTrueHandler, TFalseHandler> IfTrue(TNewTrueHandler handler) const {
        return TConditionalHandler<TPredicate, TNewTrueHandler, TFalseHandler>(Predicate, handler, FalseHandler);
    }

    template<class TNewFalseHandler>
    [[nodiscard]] TConditionalHandler<TPredicate, TTrueHandler, TNewFalseHandler> IfFalse(TNewFalseHandler handler) const {
        return TConditionalHandler<TPredicate, TTrueHandler, TNewFalseHandler>(Predicate, TrueHandler, handler);
    }

private:
    TPredicate Predicate;
    TTrueHandler TrueHandler;
    TFalseHandler FalseHandler;
};

template<class TPredicate>
TConditionalHandler<TPredicate> ConditionalHandler(TPredicate pred) {
    return TConditionalHandler<TPredicate>(pred);
}

}
