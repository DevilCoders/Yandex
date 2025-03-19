#pragma once

#include <library/cpp/neh/multiclient.h>

#include <util/generic/object_counter.h>

class IShardDelivery;

class IAddrDelivery
    : public TObjectCounter<IAddrDelivery>
    , public TGuard<IShardDelivery>
{
protected:
    IShardDelivery* Owner;
    const ui32 AttIdx;
    const TInstant StartTime;

public:
    using TPtr = TAtomicSharedPtr<IAddrDelivery>;

public:
    ui32 GetAttemption() const {
        return AttIdx;
    }

    IAddrDelivery(IShardDelivery* owner, const ui32 attIdx);
    virtual ~IAddrDelivery();

    void OnEvent(const NNeh::IMultiClient::TEvent& ev);

    virtual const NNeh::TMessage& GetMessage() const = 0;
    virtual TInstant GetDeadline() const = 0;
};

class TAddrDelivery: public IAddrDelivery {
private:
    const NNeh::TMessage Message;
    const TInstant Deadline;

public:
    TAddrDelivery(IShardDelivery* owner, const NNeh::TMessage& message, const TInstant deadline, const ui32 attIdx)
        : IAddrDelivery(owner, attIdx)
        , Message(message)
        , Deadline(deadline)
    {
    }

    virtual TInstant GetDeadline() const override {
        return Deadline;
    }

    const NNeh::TMessage& GetMessage() const override {
        return Message;
    }
};
