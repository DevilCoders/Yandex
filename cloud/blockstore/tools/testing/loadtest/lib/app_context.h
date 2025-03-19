#pragma once

#include <cloud/blockstore/tools/testing/loadtest/protos/loadtest.pb.h>

#include <cloud/blockstore/libs/client/public.h>
#include <cloud/blockstore/libs/service/public.h>
#include <cloud/blockstore/libs/validation/validation.h>

#include <util/folder/tempdir.h>
#include <util/stream/str.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/condvar.h>
#include <util/system/mutex.h>

#include <contrib/libs/sparsehash/src/sparsehash/dense_hash_map>

namespace NCloud::NBlockStore::NLoadTest {

////////////////////////////////////////////////////////////////////////////////

struct TTestContext
{
    TMutex WaitMutex;
    TCondVar WaitCondVar;
    TAtomic ShouldStop = 0;
    TAtomic Finished = 0;
    TStringStream Result;
    IBlockStorePtr Client;
    IBlockStorePtr DataClient;
    NClient::ISessionPtr Session;
    NProto::TVolume Volume;

    IBlockDigestCalculatorPtr DigestCalculator;
    using TBlockChecksums = google::dense_hash_map<ui32, ui64>;
    TBlockChecksums BlockChecksums;
};

////////////////////////////////////////////////////////////////////////////////

enum EExitCode
{
    EC_LOAD_TEST_FAILED = 1,
    EC_CONTROL_PLANE_ACTION_FAILED = 2,
    EC_VALIDATION_FAILED = 3,
    EC_COMPARE_DATA_ACTION_FAILED = 4,
    EC_TIMEOUT = 5,
    EC_FAILED_TO_LOAD_TESTS_CONFIGURATION = 6,
    EC_FAILED_TO_DESTROY_ALIASED_VOLUMES = 7,
};

////////////////////////////////////////////////////////////////////////////////

struct TAppContext
{
    TAtomic ExitCode = 0;
    TAtomic ShouldStop = 0;
    TAtomic FailedTests = 0;

    TTempDir TempDir;
};

}   // namespace NCloud::NBlockStore::NLoadTest
