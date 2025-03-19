#include "disk_agent_state.h"

#include "config.h"

#include <cloud/blockstore/config/disk.pb.h>
#include <cloud/blockstore/libs/common/block_checksum.h>
#include <cloud/blockstore/libs/common/caching_allocator.h>
#include <cloud/blockstore/libs/common/iovector.h>
#include <cloud/blockstore/libs/diagnostics/block_digest.h>
#include <cloud/blockstore/libs/diagnostics/profile_log.h>
#include <cloud/blockstore/libs/service_local/storage_null.h>
#include <cloud/blockstore/libs/service/storage_provider.h>
#include <cloud/blockstore/libs/spdk/env_stub.h>
#include <cloud/storage/core/libs/common/error.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/tempdir.h>
#include <util/generic/algorithm.h>
#include <util/string/cast.h>
#include <util/system/file.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration WaitTimeout = TDuration::Seconds(5);
constexpr ui32 DefaultDeviceBlockSize = 4096;
constexpr ui64 DefaultBlocksCount = 1024*1024;

////////////////////////////////////////////////////////////////////////////////

auto CreateTestAllocator()
{
    return CreateCachingAllocator(TDefaultAllocator::Instance(), 0, 0, 0);
}

auto CreateSpdkConfig()
{
    NProto::TDiskAgentConfig config;

    for (int i = 0; i != 3; ++i) {
        auto& device = *config.AddFileDevices();
        device.SetPath("/dev/nvme3n" + ToString(i + 1));
        device.SetBlockSize(DefaultDeviceBlockSize);
        device.SetDeviceId("uuid-" + ToString(i + 1));
    }

    return std::make_shared<TDiskAgentConfig>(std::move(config), "rack");
}

auto CreateNullConfig(TTempDir& workingDir, bool acquireRequired)
{
    NProto::TDiskAgentConfig config;
    config.SetAcquireRequired(acquireRequired);
    config.SetReleaseInactiveSessionsTimeout(10000);

    for (int i = 0; i != 3; ++i) {
        auto filePath = workingDir.Path() / ("nvme3n" + ToString(i + 1));

        TFile fileData(filePath, EOpenModeFlag::CreateNew);
        fileData.Resize(DefaultDeviceBlockSize * DefaultBlocksCount);

        auto& device = *config.AddFileDevices();
        device.SetPath(filePath);
        device.SetBlockSize(DefaultDeviceBlockSize);
        device.SetDeviceId("uuid-" + ToString(i + 1));
    }

    return std::make_shared<TDiskAgentConfig>(std::move(config), "rack");
}

auto CreateDiskAgentStateNull(TDiskAgentConfigPtr config)
{
    return std::make_unique<TDiskAgentState>(
        std::move(config),
        nullptr,    // spdk
        CreateTestAllocator(),
        NServer::CreateNullStorageProvider(),
        CreateProfileLogStub(),
        CreateBlockDigestGeneratorStub(),
        nullptr,    // logging
        nullptr);   // rdmaServer
}

auto CreateDiskAgentStateSpdk(TDiskAgentConfigPtr config)
{
    return std::make_unique<TDiskAgentState>(
        std::move(config),
        NSpdk::CreateEnvStub(),
        CreateTestAllocator(),
        nullptr,    // storageProvider
        CreateProfileLogStub(),
        CreateBlockDigestGeneratorStub(),
        nullptr,    // logging
        nullptr);   // rdmaServer
}

////////////////////////////////////////////////////////////////////////////////

struct TStorageProvider: IStorageProvider
{
    THashSet<TString> Paths;

    TStorageProvider(THashSet<TString> paths)
        : Paths(std::move(paths))
    {
    }

    NThreading::TFuture<IStoragePtr> CreateStorage(
        const NProto::TVolume& volume,
        const TString& clientId,
        NProto::EVolumeAccessMode accessMode) override
    {
        Y_UNUSED(clientId);
        Y_UNUSED(accessMode);

        if (Paths.contains(volume.GetDiskId())) {
            return MakeFuture(NServer::CreateNullStorage());
        }

        return MakeErrorFuture<IStoragePtr>(
            std::make_exception_ptr(yexception() << "oops")
        );
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TDiskAgentStateTest)
{
    void ShouldInitialize(TDiskAgentState& state)
    {
        auto r = state.Initialize().GetValue(WaitTimeout);

        UNIT_ASSERT(r.Errors.empty());
        UNIT_ASSERT_VALUES_EQUAL(3, r.Configs.size());

        auto stats = state.CollectStats().GetValue(WaitTimeout);

        UNIT_ASSERT_VALUES_EQUAL(0, stats.GetInitErrorsCount());
        UNIT_ASSERT_VALUES_EQUAL(3, stats.GetDeviceStats().size());

        for (int i = 0; i != 3; ++i) {
            const TString expected = "uuid-" + ToString(i + 1);

            UNIT_ASSERT_VALUES_EQUAL(expected, r.Configs[i].GetDeviceUUID());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultDeviceBlockSize,
                r.Configs[i].GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultBlocksCount,
                r.Configs[i].GetBlocksCount());
        }

        {
            NProto::TReadDeviceBlocksRequest request;
            request.SetDeviceUUID("uuid-1");
            request.SetStartIndex(1);
            request.SetBlockSize(4096);
            request.SetBlocksCount(10);

            auto response = state.Read(Now(), std::move(request))
                .GetValue(WaitTimeout);

            UNIT_ASSERT(!HasError(response));
            UNIT_ASSERT_VALUES_EQUAL(10, response.GetBlocks().BuffersSize());
            for (const auto& buffer: response.GetBlocks().GetBuffers()) {
                UNIT_ASSERT_VALUES_EQUAL(4096, buffer.size());
            }
        }

        {
            NProto::TWriteDeviceBlocksRequest request;
            request.SetDeviceUUID("uuid-1");
            request.SetStartIndex(1);
            request.SetBlockSize(4096);

            ResizeIOVector(*request.MutableBlocks(), 10, 4096);

            auto response = state.Write(Now(), std::move(request))
                .GetValue(WaitTimeout);

            UNIT_ASSERT(!HasError(response));
        }

        {
            NProto::TZeroDeviceBlocksRequest request;
            request.SetDeviceUUID("uuid-1");
            request.SetStartIndex(1);
            request.SetBlockSize(4096);
            request.SetBlocksCount(10);

            auto response = state.WriteZeroes(Now(), std::move(request))
                .GetValue(WaitTimeout);

            UNIT_ASSERT(!HasError(response));
        }

        {
            auto error = state.SecureErase("uuid-1", {}).GetValue(WaitTimeout);
            UNIT_ASSERT(!HasError(error));
        }
    }

    Y_UNIT_TEST(ShouldInitializeWithSpdk)
    {
        auto state = CreateDiskAgentStateSpdk(
            CreateSpdkConfig());

        ShouldInitialize(*state);
    }

    Y_UNIT_TEST(ShouldInitializeWithNull)
    {
        TTempDir workingDir;

        auto state = CreateDiskAgentStateNull(
            CreateNullConfig(workingDir, false)
        );

        ShouldInitialize(*state);
    }

    Y_UNIT_TEST(ShouldProperlyProcessRequestsToUninitializedDevices)
    {
        TTempDir workingDir;

        auto config = CreateNullConfig(workingDir, false);

        TDiskAgentState state(
            config,
            nullptr,    // spdk
            CreateTestAllocator(),
            std::make_shared<TStorageProvider>(THashSet<TString>{
                config->GetFileDevices()[0].GetPath(),
                config->GetFileDevices()[1].GetPath(),
            }),
            CreateProfileLogStub(),
            CreateBlockDigestGeneratorStub(),
            nullptr,    // logging
            nullptr);   // rdmaServer

        auto r = state.Initialize().GetValue(WaitTimeout);

        UNIT_ASSERT_VALUES_EQUAL(1, r.Errors.size());
        UNIT_ASSERT_VALUES_EQUAL(3, r.Configs.size());

        auto stats = state.CollectStats().GetValue(WaitTimeout);

        UNIT_ASSERT_VALUES_EQUAL(1, stats.GetInitErrorsCount());
        UNIT_ASSERT_VALUES_EQUAL(3, stats.GetDeviceStats().size());

        for (int i = 0; i != 3; ++i) {
            const TString expected = "uuid-" + ToString(i + 1);

            UNIT_ASSERT_VALUES_EQUAL(expected, r.Configs[i].GetDeviceUUID());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultDeviceBlockSize,
                r.Configs[i].GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultBlocksCount,
                r.Configs[i].GetBlocksCount());
        }

        const TString sessionId = "session";

        try {
            state.AcquireDevices(
                {"uuid-1", "uuid-2", "uuid-3"},
                sessionId,
                TInstant::Seconds(1),
                NProto::VOLUME_ACCESS_READ_WRITE,
                0
            );
        } catch (const TServiceError& e) {
            UNIT_ASSERT_C(false, e.GetMessage());
        }

        {
            NProto::TReadDeviceBlocksRequest request;
            request.SetDeviceUUID("uuid-1");
            request.SetStartIndex(1);
            request.SetBlockSize(4096);
            request.SetBlocksCount(10);
            request.SetSessionId(sessionId);

            auto response = state.Read(Now(), std::move(request))
                .GetValue(WaitTimeout);

            UNIT_ASSERT(!HasError(response));
            UNIT_ASSERT_VALUES_EQUAL(10, response.GetBlocks().BuffersSize());
            for (const auto& buffer: response.GetBlocks().GetBuffers()) {
                UNIT_ASSERT_VALUES_EQUAL(4096, buffer.size());
            }
        }

        {
            NProto::TWriteDeviceBlocksRequest request;
            request.SetDeviceUUID("uuid-1");
            request.SetStartIndex(1);
            request.SetBlockSize(4096);
            request.SetSessionId(sessionId);

            ResizeIOVector(*request.MutableBlocks(), 10, 4096);

            auto response = state.Write(Now(), std::move(request))
                .GetValue(WaitTimeout);

            UNIT_ASSERT(!HasError(response));
        }

        {
            NProto::TZeroDeviceBlocksRequest request;
            request.SetDeviceUUID("uuid-1");
            request.SetStartIndex(1);
            request.SetBlockSize(4096);
            request.SetBlocksCount(10);
            request.SetSessionId(sessionId);

            auto response = state.WriteZeroes(Now(), std::move(request))
                .GetValue(WaitTimeout);

            UNIT_ASSERT(!HasError(response));
        }

        try {
            state.ReleaseDevices({"uuid-1"}, sessionId);
        } catch (const TServiceError& e) {
            UNIT_ASSERT_C(false, e.GetMessage());
        }

        {
            auto error = state.SecureErase("uuid-1", {}).GetValue(WaitTimeout);
            UNIT_ASSERT(!HasError(error));
        }

        try {
            NProto::TReadDeviceBlocksRequest request;
            request.SetDeviceUUID("uuid-3");
            request.SetStartIndex(1);
            request.SetBlockSize(4096);
            request.SetBlocksCount(10);
            request.SetSessionId(sessionId);

            state.Read(Now(), std::move(request)).GetValue(WaitTimeout);

            UNIT_ASSERT(false);
        } catch (const TServiceError& e) {
            UNIT_ASSERT_VALUES_EQUAL_C(
                E_IO,
                e.GetCode(),
                e.GetMessage()
            );
        }

        try {
            NProto::TWriteDeviceBlocksRequest request;
            request.SetDeviceUUID("uuid-3");
            request.SetStartIndex(1);
            request.SetBlockSize(4096);
            request.SetSessionId(sessionId);

            ResizeIOVector(*request.MutableBlocks(), 10, 4096);

            state.Write(Now(), std::move(request)).GetValue(WaitTimeout);

            UNIT_ASSERT(false);
        } catch (const TServiceError& e) {
            UNIT_ASSERT_VALUES_EQUAL_C(
                E_IO,
                e.GetCode(),
                e.GetMessage()
            );
        }

        try {
            NProto::TZeroDeviceBlocksRequest request;
            request.SetDeviceUUID("uuid-3");
            request.SetStartIndex(1);
            request.SetBlockSize(4096);
            request.SetBlocksCount(10);
            request.SetSessionId(sessionId);

            state.WriteZeroes(Now(), std::move(request)).GetValue(WaitTimeout);

            UNIT_ASSERT(false);
        } catch (const TServiceError& e) {
            UNIT_ASSERT_VALUES_EQUAL_C(
                E_IO,
                e.GetCode(),
                e.GetMessage()
            );
        }

        try {
            state.ReleaseDevices(
                {"uuid-2", "uuid-3"},
                sessionId
            );
        } catch (const TServiceError& e) {
            UNIT_ASSERT_C(false, e.GetMessage());
        }

        try {
            state.SecureErase("uuid-3", {}).GetValue(WaitTimeout);

            UNIT_ASSERT(false);
        } catch (const TServiceError& e) {
            UNIT_ASSERT_VALUES_EQUAL_C(
                E_IO,
                e.GetCode(),
                e.GetMessage()
            );
        }
    }

    Y_UNIT_TEST(ShouldCountInitErrors)
    {
        NProto::TDiskAgentConfig config;

        for (int i = 0; i != 3; ++i) {
            auto& device = *config.AddFileDevices();
            device.SetPath("/dev/not-exists-" + ToString(i + 1));
            device.SetBlockSize(DefaultDeviceBlockSize);
            device.SetDeviceId("uuid-" + ToString(i + 1));
        }

        TDiskAgentState state(
            std::make_shared<TDiskAgentConfig>(std::move(config), "rack"),
            nullptr,    // spdk
            CreateTestAllocator(),
            NServer::CreateNullStorageProvider(),
            CreateProfileLogStub(),
            CreateBlockDigestGeneratorStub(),
            nullptr,    // logging
            nullptr);   // rdmaServer

        auto r = state.Initialize().GetValue(WaitTimeout);

        UNIT_ASSERT_VALUES_EQUAL(3, r.Errors.size());
        UNIT_ASSERT_VALUES_EQUAL(3, r.Configs.size());

        auto stats = state.CollectStats().GetValue(WaitTimeout);

        UNIT_ASSERT_VALUES_EQUAL(3, stats.GetInitErrorsCount());
        UNIT_ASSERT_VALUES_EQUAL(3, stats.GetDeviceStats().size());
    }

    Y_UNIT_TEST(ShouldAcquireDevices)
    {
        TTempDir workingDir;

        auto state = CreateDiskAgentStateNull(
            CreateNullConfig(workingDir, true)
        );

        auto r = state->Initialize().GetValue(WaitTimeout);
        UNIT_ASSERT(r.Errors.empty());

#define TEST_SHOULD_READ(sessionId)                                            \
        try {                                                                  \
            NProto::TReadDeviceBlocksRequest request;                          \
            request.SetDeviceUUID("uuid-1");                                   \
            request.SetStartIndex(1);                                          \
            request.SetBlockSize(4096);                                        \
            request.SetBlocksCount(10);                                        \
            request.SetSessionId(sessionId);                                   \
                                                                               \
            auto response =                                                    \
                state->Read(                                                   \
                    Now(),                                                     \
                    std::move(request)                                         \
                ).GetValue(WaitTimeout);                                       \
            UNIT_ASSERT_VALUES_EQUAL_C(                                        \
                S_OK,                                                          \
                response.GetError().GetCode(),                                 \
                response.GetError().GetMessage()                               \
            );                                                                 \
        } catch (const TServiceError& e) {                                     \
            UNIT_ASSERT_C(false, e.GetMessage());                              \
        }                                                                      \
// TEST_SHOULD_READ

#define TEST_SHOULD_WRITE(sessionId)                                           \
        try {                                                                  \
            NProto::TWriteDeviceBlocksRequest request;                         \
            request.SetDeviceUUID("uuid-1");                                   \
            request.SetStartIndex(1);                                          \
            request.SetBlockSize(4096);                                        \
            request.SetSessionId(sessionId);                                   \
                                                                               \
            ResizeIOVector(*request.MutableBlocks(), 10, 4096);                \
                                                                               \
            auto response =                                                    \
                state->Write(                                                  \
                    Now(),                                                     \
                    std::move(request)                                         \
                ).GetValue(WaitTimeout);                                       \
            UNIT_ASSERT_VALUES_EQUAL_C(                                        \
                S_OK,                                                          \
                response.GetError().GetCode(),                                 \
                response.GetError().GetMessage()                               \
            );                                                                 \
        } catch (const TServiceError& e) {                                     \
            UNIT_ASSERT_C(false, e.GetMessage());                              \
        }                                                                      \
// TEST_SHOULD_WRITE

#define TEST_SHOULD_ZERO(sessionId)                                            \
        try {                                                                  \
            NProto::TZeroDeviceBlocksRequest request;                          \
            request.SetDeviceUUID("uuid-1");                                   \
            request.SetStartIndex(1);                                          \
            request.SetBlockSize(4096);                                        \
            request.SetBlocksCount(10);                                        \
            request.SetSessionId(sessionId);                                   \
                                                                               \
            auto response =                                                    \
                state->WriteZeroes(                                            \
                    Now(),                                                     \
                    std::move(request)                                         \
                ).GetValue(WaitTimeout);                                       \
            UNIT_ASSERT_VALUES_EQUAL_C(                                        \
                S_OK,                                                          \
                response.GetError().GetCode(),                                 \
                response.GetError().GetMessage()                               \
            );                                                                 \
        } catch (const TServiceError& e) {                                     \
            UNIT_ASSERT_C(false, e.GetMessage());                              \
        }                                                                      \
// TEST_SHOULD_ZERO

#define TEST_SHOULD_NOT_READ(sessionId)                                        \
        try {                                                                  \
            NProto::TReadDeviceBlocksRequest request;                          \
            request.SetDeviceUUID("uuid-1");                                   \
            request.SetStartIndex(1);                                          \
            request.SetBlockSize(4096);                                        \
            request.SetBlocksCount(10);                                        \
            request.SetSessionId(sessionId);                                   \
                                                                               \
            auto response =                                                    \
                state->Read(                                                   \
                    Now(),                                                     \
                    std::move(request)                                         \
                ).GetValue(WaitTimeout);                                       \
            UNIT_ASSERT(false);                                                \
        } catch (const TServiceError& e) {                                     \
            UNIT_ASSERT_VALUES_EQUAL_C(                                        \
                E_BS_INVALID_SESSION,                                          \
                e.GetCode(),                                                   \
                e.GetMessage()                                                 \
            );                                                                 \
        }                                                                      \
// TEST_SHOULD_NOT_READ

#define TEST_SHOULD_NOT_WRITE(sessionId)                                       \
        try {                                                                  \
            NProto::TWriteDeviceBlocksRequest request;                         \
            request.SetDeviceUUID("uuid-1");                                   \
            request.SetStartIndex(1);                                          \
            request.SetBlockSize(4096);                                        \
            request.SetSessionId(sessionId);                                   \
                                                                               \
            ResizeIOVector(*request.MutableBlocks(), 10, 4096);                \
                                                                               \
            auto response =                                                    \
                state->Write(                                                  \
                    Now(),                                                     \
                    std::move(request)                                         \
                ).GetValue(WaitTimeout);                                       \
            UNIT_ASSERT(false);                                                \
        } catch (const TServiceError& e) {                                     \
            UNIT_ASSERT_VALUES_EQUAL_C(                                        \
                E_BS_INVALID_SESSION,                                          \
                e.GetCode(),                                                   \
                e.GetMessage()                                                 \
            );                                                                 \
        }                                                                      \
// TEST_SHOULD_NOT_WRITE

#define TEST_SHOULD_NOT_ZERO(sessionId)                                        \
        try {                                                                  \
            NProto::TZeroDeviceBlocksRequest request;                          \
            request.SetDeviceUUID("uuid-1");                                   \
            request.SetStartIndex(1);                                          \
            request.SetBlockSize(4096);                                        \
            request.SetBlocksCount(10);                                        \
            request.SetSessionId(sessionId);                                   \
                                                                               \
            auto response =                                                    \
                state->WriteZeroes(                                            \
                    Now(),                                                     \
                    std::move(request)                                         \
                ).GetValue(WaitTimeout);                                       \
            UNIT_ASSERT(false);                                                \
        } catch (const TServiceError& e) {                                     \
            UNIT_ASSERT_VALUES_EQUAL_C(                                        \
                E_BS_INVALID_SESSION,                                          \
                e.GetCode(),                                                   \
                e.GetMessage()                                                 \
            );                                                                 \
        }                                                                      \
// TEST_SHOULD_NOT_READ

#define TEST_SHOULD_ACQUIRE(sessionId, ts, mode, seqNo)                        \
        try {                                                                  \
            state->AcquireDevices(                                             \
                {"uuid-1"},                                                    \
                sessionId,                                                     \
                ts,                                                            \
                mode,                                                          \
                seqNo                                                          \
            );                                                                 \
        } catch (const TServiceError& e) {                                     \
            UNIT_ASSERT_C(false, e.GetMessage());                              \
        }                                                                      \
// TEST_SHOULD_ACQUIRE

#define TEST_SHOULD_NOT_ACQUIRE(sessionId, ts, mode, seqNo)                    \
        try {                                                                  \
            state->AcquireDevices(                                             \
                {"uuid-1"},                                                    \
                sessionId,                                                     \
                ts,                                                            \
                mode,                                                          \
                seqNo                                                          \
            );                                                                 \
            UNIT_ASSERT(false);                                                \
        } catch (const TServiceError& e) {                                     \
            UNIT_ASSERT_VALUES_EQUAL_C(                                        \
                E_BS_INVALID_SESSION,                                          \
                e.GetCode(),                                                   \
                e.GetMessage()                                                 \
            );                                                                 \
        }                                                                      \
// TEST_SHOULD_NOT_ACQUIRE

        // read requests should fail for unregistered sessions
        TEST_SHOULD_NOT_READ("some-session");

        // write requests should also fail
        TEST_SHOULD_NOT_WRITE("some-session");

        // and zero requests as well
        TEST_SHOULD_NOT_ZERO("some-session");

        // secure erase should succeed as long as there are no active sessions
        {
            auto error = state->SecureErase("uuid-1", {}).GetValue(WaitTimeout);
            UNIT_ASSERT(!HasError(error));
        }

        // first rw acquire should succeed
        TEST_SHOULD_ACQUIRE(
            "writer",
            TInstant::Seconds(1),
            NProto::VOLUME_ACCESS_READ_WRITE,
            0
        );

        // there is an active rw session already - second writer should be
        // rejected
        TEST_SHOULD_NOT_ACQUIRE(
            "writer2",
            TInstant::Seconds(1),
            NProto::VOLUME_ACCESS_READ_WRITE,
            0
        );

        // but readers can register
        TEST_SHOULD_ACQUIRE(
            "reader1",
            TInstant::Seconds(1),
            NProto::VOLUME_ACCESS_READ_ONLY,
            0
        );

        // the number of readers is not limited by anything
        TEST_SHOULD_ACQUIRE(
            "reader2",
            TInstant::Seconds(1),
            NProto::VOLUME_ACCESS_READ_ONLY,
            0
        );

        // all sessions should be able to read
        for (auto sessId: {"writer", "reader1", "reader2"}) {
            TEST_SHOULD_READ(sessId);
        }

        // but only writer should be able to write
        TEST_SHOULD_WRITE("writer");

        // and to zero
        TEST_SHOULD_ZERO("writer");

        // readers should not be allowed to write
        TEST_SHOULD_NOT_WRITE("reader1");
        TEST_SHOULD_NOT_WRITE("reader2");

        // and to zero
        TEST_SHOULD_NOT_ZERO("reader1");
        TEST_SHOULD_NOT_ZERO("reader2");

        // secure erase should fail - there are active sessions
        try {
            auto error = state->SecureErase(
                "uuid-1",
                TInstant::Seconds(1)
            ).GetValue(WaitTimeout);
            UNIT_ASSERT(false);
        } catch (const TServiceError& e) {
            UNIT_ASSERT_VALUES_EQUAL_C(
                E_INVALID_STATE,
                e.GetCode(),
                e.GetMessage()
            );
        }

        // after the inactivity period secure erase should succeed - our
        // sessions are no longer considered 'active'
        try {
            auto error = state->SecureErase(
                "uuid-1",
                TInstant::Seconds(111)
            ).GetValue(WaitTimeout);
        } catch (const TServiceError& e) {
            UNIT_ASSERT_C(false, e.GetMessage());
        }

        // second writer should be able to register - previous writer is no
        // longer active (due to the inactivity period)
        TEST_SHOULD_ACQUIRE(
            "writer2",
            TInstant::Seconds(11),
            NProto::VOLUME_ACCESS_READ_WRITE,
            0
        );

        // reacquire should succeed
        TEST_SHOULD_ACQUIRE(
            "writer2",
            TInstant::Seconds(16),
            NProto::VOLUME_ACCESS_READ_WRITE,
            0
        );

        // previous writer cannot register - there is a new writer already
        TEST_SHOULD_NOT_ACQUIRE(
            "writer",
            TInstant::Seconds(21),
            NProto::VOLUME_ACCESS_READ_WRITE,
            0
        );

        // new writer should be able to write
        TEST_SHOULD_WRITE("writer2");

        // readonly reacquire for a readwrite session is fine
        TEST_SHOULD_ACQUIRE(
            "writer2",
            TInstant::Seconds(21),
            NProto::VOLUME_ACCESS_READ_ONLY,
            0
        );

        // writer2 should be unable to write/zero now
        TEST_SHOULD_NOT_WRITE("writer2");
        TEST_SHOULD_NOT_ZERO("writer2");

        // now our other writer should be able to register
        TEST_SHOULD_ACQUIRE(
            "writer",
            TInstant::Seconds(22),
            NProto::VOLUME_ACCESS_READ_WRITE,
            0
        );

        // and should be able to write
        TEST_SHOULD_WRITE("writer");
        TEST_SHOULD_ZERO("writer");

        // after writer's session expires, writer2 should be able to become a
        // writer
        TEST_SHOULD_ACQUIRE(
            "writer2",
            TInstant::Seconds(32),
            NProto::VOLUME_ACCESS_READ_WRITE,
            0
        );

        // and should be able to write
        TEST_SHOULD_WRITE("writer2");
        TEST_SHOULD_ZERO("writer2");

        // whereas writer should not be able to write
        TEST_SHOULD_NOT_WRITE("writer");
        TEST_SHOULD_NOT_ZERO("writer");

        // writer3 should be able to acquire session with a greater seqno
        TEST_SHOULD_ACQUIRE(
            "writer3",
            TInstant::Seconds(33),
            NProto::VOLUME_ACCESS_READ_WRITE,
            1
        );

        // writer4 should be able to acquire session with a greater seqno
        TEST_SHOULD_ACQUIRE(
            "writer4",
            TInstant::Seconds(34),
            NProto::VOLUME_ACCESS_READ_WRITE,
            2
        );

        // writer3 should not be able to acquire session with a lower seqno
        TEST_SHOULD_NOT_ACQUIRE(
            "writer3",
            TInstant::Seconds(35),
            NProto::VOLUME_ACCESS_READ_WRITE,
            1
        );

        // writer4 should be able to reacquire session with same seqno
        TEST_SHOULD_ACQUIRE(
            "writer4",
            TInstant::Seconds(36),
            NProto::VOLUME_ACCESS_READ_WRITE,
            2
        );
    }
}

}   // namespace NCloud::NBlockStore::NStorage
