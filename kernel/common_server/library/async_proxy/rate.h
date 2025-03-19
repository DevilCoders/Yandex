#pragma once

#include <util/generic/ptr.h>
#include <library/cpp/deprecated/atomic/atomic.h>

namespace NJson {
    class TJsonValue;
}

class IReaskLimiter {
public:
    virtual ~IReaskLimiter() {
    }

    virtual bool CanReask() const = 0;

    virtual void OnReply(const bool isReask) = 0;
    virtual void OnRequest(const bool isReask) = 0;
    virtual NJson::TJsonValue GetReport() const = 0;
};

using TReaskLimiterPtr = TAtomicSharedPtr<IReaskLimiter>;

class TSimpleReaskLimiter: public IReaskLimiter {
public:
    TSimpleReaskLimiter(float rate = 0.25);
    ~TSimpleReaskLimiter();

    virtual void OnRequest(const bool isReask) override;
    virtual void OnReply(const bool isReask) override;
    virtual bool CanReask() const override;
    virtual NJson::TJsonValue GetReport() const override;

private:
    const float Rate;

    TAtomic ReasksCount = 0;
    TAtomic RequestsCount = 0;
};
