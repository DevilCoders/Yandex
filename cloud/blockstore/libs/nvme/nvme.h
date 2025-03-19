#pragma once

#include "public.h"

#include "spec.h"

#include <cloud/storage/core/libs/common/error.h>

namespace NCloud::NBlockStore::NNvme {

////////////////////////////////////////////////////////////////////////////////

struct INvmeManager
{
    virtual ~INvmeManager() = default;

    virtual NThreading::TFuture<NProto::TError> Format(
        const TString& path,
        nvme_secure_erase_setting ses) = 0;
};

////////////////////////////////////////////////////////////////////////////////

INvmeManagerPtr CreateNvmeManager(TDuration timeout);
INvmeManagerPtr CreateNvmeManagerStub();

}   // namespace NCloud::NBlockStore::NNvme
