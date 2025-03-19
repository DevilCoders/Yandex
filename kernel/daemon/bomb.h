#pragma once

#include <util/thread/factory.h>
#include <util/system/event.h>

class TBomb : public IThreadFactory::IThreadAble {
public:
    TBomb(const TDuration& timeout, const TString& message = TString());
    virtual ~TBomb() override;
    void Deactivate();
private:
    virtual void DoExecute() override;
    TInstant Deadline;
    TSystemEvent Deactivated;
    TString Message;
    TAutoPtr<IThreadFactory::IThread> Thread;
};
