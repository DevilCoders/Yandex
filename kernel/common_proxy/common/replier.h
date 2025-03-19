#pragma once

#include "data_set.h"
#include <library/cpp/json/json_value.h>

namespace NCommonProxy {

    class IReplier : public TAtomicRefCount<IReplier> {
    public:
        using TPtr = TIntrusivePtr<IReplier>;

    public:
        virtual ~IReplier();
        virtual void AddReply(const TString& processorName, int code = 200, const TString& message = Default<TString>(), TDataSet::TPtr data = nullptr) = 0;
        virtual bool Canceled() const;
        virtual void AddTrace(const TString& comment);
        virtual void AddMessage(const TString& processorName, int code, const NJson::TJsonValue& msg);
        virtual TInstant GetStartTime() const;

    private:
        TInstant StartTime = Now();
    };

}
