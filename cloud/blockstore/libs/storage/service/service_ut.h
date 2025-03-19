#pragma once

#include <cloud/blockstore/config/features.pb.h>

#include <cloud/blockstore/libs/diagnostics/public.h>

#include <cloud/blockstore/libs/storage/testlib/service_client.h>
#include <cloud/blockstore/libs/storage/testlib/test_env.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

TDiagnosticsConfigPtr CreateTestDiagnosticsConfig();

ui32 SetupTestEnv(TTestEnv& env);
ui32 SetupTestEnv(
    TTestEnv& env,
    NProto::TStorageServiceConfig storageServiceConfig,
    NProto::TFeaturesConfig featuresConfig = {});

ui32 SetupTestEnvWithMultipleMount(
    TTestEnv& env,
    TDuration inactivateTimeout);

ui32 SetupTestEnvWithMultipleMountAndTwoStageLocalMount(
    TTestEnv& env,
    TDuration inactivateTimeout);

ui32 SetupTestEnvWithAllowVersionInModifyScheme(TTestEnv& env);

TString GetBlockContent(char fill, size_t size = DefaultBlockSize);

}   // namespace NCloud::NBlockStore::NStorage
