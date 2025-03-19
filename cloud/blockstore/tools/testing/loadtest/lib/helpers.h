#pragma once

#include "app_context.h"

#include <cloud/storage/core/libs/common/error.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/string/printf.h>
#include <library/cpp/deprecated/atomic/atomic.h>

namespace NCloud::NBlockStore::NLoadTest {

////////////////////////////////////////////////////////////////////////////////

inline TString MakeLoggingTag(const TString& testName)
{
    return Sprintf("[%s] ", testName.c_str());
}

////////////////////////////////////////////////////////////////////////////////

inline void StopTest(TTestContext& testContext)
{
    AtomicSet(testContext.ShouldStop, 1);
    testContext.WaitCondVar.Signal();
}

////////////////////////////////////////////////////////////////////////////////

template <typename T>
T WaitForCompletion(
    const TString& request,
    const NThreading::TFuture<T>& future,
    const TVector<ui32>& successOnError)
{
    const auto& response = future.GetValue(TDuration::Max());
    if (HasError(response)) {
        const auto& error = response.GetError();
        if (Find(successOnError, error.GetCode()) == successOnError.end()) {
            throw yexception()
                << "Failed to execute " << request << " request: "
                << FormatError(error);
        }
    }
    return response;
}

}   // namespace NCloud::NBlockStore::NLoadTest
