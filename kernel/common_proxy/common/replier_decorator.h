#pragma once
#include <kernel/common_proxy/common/replier.h>

namespace NCommonProxy {
    class TReplierDecorator : public IReplier {
    public:
        explicit TReplierDecorator(IReplier::TPtr slave);

        virtual void AddReply(const TString& processorName, int code, const TString& message, TDataSet::TPtr data) override;
        virtual TInstant GetStartTime() const override;
        virtual bool Canceled() const override;
        virtual void AddTrace(const TString& comment) override;

    protected:
        IReplier::TPtr Slave;
    };
}
