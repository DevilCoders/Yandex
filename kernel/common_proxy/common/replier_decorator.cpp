#include <kernel/common_proxy/common/replier_decorator.h>

using namespace NCommonProxy;

TReplierDecorator::TReplierDecorator(IReplier::TPtr slave)
    : Slave(slave)
{
}

void TReplierDecorator::AddReply(const TString& processorName, int code, const TString& message, TDataSet::TPtr data) {
    Slave->AddReply(processorName, code, message, data);
}

TInstant TReplierDecorator::GetStartTime() const {
    return Slave->GetStartTime();
}

bool TReplierDecorator::Canceled() const {
    return Slave->Canceled();
}

void NCommonProxy::TReplierDecorator::AddTrace(const TString& comment) {
    Slave->AddTrace(comment);
}
