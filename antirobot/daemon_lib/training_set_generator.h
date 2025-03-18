#pragma once

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>

namespace NAntiRobot {

class TRequest;
struct TRequestContext;

class ITrainingSetGenerator
{
public:
    enum EResult
    {
        AddedRandomCaptchaRequest,
        AddedRandomUserRequest,
        RequestNotUsed,
    };

    virtual EResult ProcessRequest(const TRequestContext& rc) = 0;
    virtual TStringBuf Name() = 0;
    virtual void ProcessCaptchaGenerationError(const TRequestContext& /*rc*/) {
    }

protected:
    ITrainingSetGenerator() {
    }

    virtual ~ITrainingSetGenerator() {
    }
};

ITrainingSetGenerator* NullTrainingSetGenerator();
ITrainingSetGenerator* RandomUserTrainingSetGenerator();

} /* namespace NAntiRobot */
