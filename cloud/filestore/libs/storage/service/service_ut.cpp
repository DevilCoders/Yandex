#include "service.h"

#include <cloud/filestore/libs/storage/api/ss_proxy.h>
#include <cloud/filestore/libs/storage/api/tablet.h>
#include <cloud/filestore/libs/storage/testlib/service_client.h>
#include <cloud/filestore/libs/storage/testlib/tablet_client.h>
#include <cloud/filestore/libs/storage/testlib/test_env.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;
using namespace NKikimr;

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TStorageServiceTest)
{
    Y_UNIT_TEST(ShouldCreateFileStore)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("test", 1000);

        auto response = service.GetFileStoreInfo("test")->Record.GetFileStore();
        UNIT_ASSERT_VALUES_EQUAL(response.GetFileSystemId(), "test");
        UNIT_ASSERT_VALUES_EQUAL(response.GetCloudId(), "test");
        UNIT_ASSERT_VALUES_EQUAL(response.GetFolderId(), "test");
        UNIT_ASSERT_VALUES_EQUAL(response.GetBlocksCount(), 1000);
        UNIT_ASSERT_VALUES_EQUAL(response.GetBlockSize(), DefaultBlockSize);
        UNIT_ASSERT_VALUES_EQUAL(response.GetConfigVersion(), 1);

        service.DestroyFileStore("test");
        service.AssertGetFileStoreInfoFailed("test");
    }

    Y_UNIT_TEST(ShouldAlterFileStore)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("test", 1000);
        service.AlterFileStore("test", "yyyy", "zzzz");

        auto response = service.GetFileStoreInfo("test")->Record.GetFileStore();
        UNIT_ASSERT_VALUES_EQUAL(response.GetFileSystemId(), "test");
        UNIT_ASSERT_VALUES_EQUAL(response.GetCloudId(), "yyyy");
        UNIT_ASSERT_VALUES_EQUAL(response.GetFolderId(), "zzzz");
        UNIT_ASSERT_VALUES_EQUAL(response.GetBlocksCount(), 1000);
        UNIT_ASSERT_VALUES_EQUAL(response.GetBlockSize(), DefaultBlockSize);
        UNIT_ASSERT_VALUES_EQUAL(response.GetConfigVersion(), 2);
    }

    Y_UNIT_TEST(ShouldResizeFileStore)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("test", 1000);
        service.ResizeFileStore("test", 2000);

        auto response = service.GetFileStoreInfo("test")->Record.GetFileStore();
        UNIT_ASSERT_VALUES_EQUAL(response.GetFileSystemId(), "test");
        UNIT_ASSERT_VALUES_EQUAL(response.GetCloudId(), "test");
        UNIT_ASSERT_VALUES_EQUAL(response.GetFolderId(), "test");
        UNIT_ASSERT_VALUES_EQUAL(response.GetBlocksCount(), 2000);
        UNIT_ASSERT_VALUES_EQUAL(response.GetBlockSize(), DefaultBlockSize);
        UNIT_ASSERT_VALUES_EQUAL(response.GetConfigVersion(), 2);

        service.AssertResizeFileStoreFailed("test", 1000);
        service.AssertResizeFileStoreFailed("test", 0);
    }

    Y_UNIT_TEST(ShouldResizeFileStoreAndAddChannels)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");
        ui32 nodeIdx = env.CreateNode("nfs");

        auto& runtime = env.GetRuntime();

        ui32 createChannelsCount = 0;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvModifySchemeRequest: {
                        auto* msg = event->Get<TEvSSProxy::TEvModifySchemeRequest>();
                        if (msg->ModifyScheme.GetOperationType() ==
                            NKikimrSchemeOp::ESchemeOpCreateFileStore)
                        {
                            const auto& request = msg->ModifyScheme.GetCreateFileStore();
                            const auto& config = request.GetConfig();
                            createChannelsCount = config.ExplicitChannelProfilesSize();
                        }
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("test", 1000);
        UNIT_ASSERT(createChannelsCount > 0);

        ui32 alterChannelsCount = 0;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvModifySchemeRequest: {
                        auto* msg = event->Get<TEvSSProxy::TEvModifySchemeRequest>();
                        if (msg->ModifyScheme.GetOperationType() ==
                            NKikimrSchemeOp::ESchemeOpAlterFileStore)
                        {
                            const auto& request = msg->ModifyScheme.GetAlterFileStore();
                            const auto& config = request.GetConfig();
                            alterChannelsCount = config.ExplicitChannelProfilesSize();
                        }
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });
        service.ResizeFileStore("test", 4_TB/DefaultBlockSize);
        UNIT_ASSERT(alterChannelsCount > 0);
        UNIT_ASSERT(alterChannelsCount > createChannelsCount);
    }

    Y_UNIT_TEST(ShouldFailAlterIfDescribeFails)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");
        ui32 nodeIdx = env.CreateNode("nfs");

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateFileStore("test", 1000);

        auto error = MakeError(E_ARGUMENT, "Error");
        runtime.SetObserverFunc(
            [=] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvDescribeFileStoreRequest: {
                        auto response = std::make_unique<TEvSSProxy::TEvDescribeFileStoreResponse>(
                            error);
                        runtime.Send(
                            new IEventHandle(
                                event->Sender,
                                event->Recipient,
                                response.release(),
                                0, // flags
                                event->Cookie),
                            nodeIdx);
                        return TTestActorRuntime::EEventAction::DROP;
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.AssertAlterFileStoreFailed("test", "xxxx", "yyyy");
    }

    Y_UNIT_TEST(ShouldDescribeModel)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TServiceClient service(env.GetRuntime(), nodeIdx);
        const auto size1 = 1_GB/DefaultBlockSize;

        auto response1 = service.DescribeFileStoreModel(size1);
        auto& model1 = response1->Record.GetFileStoreModel();
        UNIT_ASSERT_VALUES_EQUAL(model1.GetBlocksCount(), size1);
        UNIT_ASSERT_VALUES_EQUAL(model1.GetBlockSize(), DefaultBlockSize);
        UNIT_ASSERT_VALUES_EQUAL(model1.GetChannelsCount(), 7);

        auto& profile1 = model1.GetPerformanceProfile();
        UNIT_ASSERT_VALUES_EQUAL(profile1.GetMaxReadIops(), 100);
        UNIT_ASSERT_VALUES_EQUAL(profile1.GetMaxWriteIops(), 300);
        UNIT_ASSERT_VALUES_EQUAL(profile1.GetMaxReadBandwidth(), 30_MB);
        UNIT_ASSERT_VALUES_EQUAL(profile1.GetMaxWriteBandwidth(), 30_MB);

        const auto size2 = 4_TB/DefaultBlockSize;
        auto response2 = service.DescribeFileStoreModel(size2);
        auto& model2 = response2->Record.GetFileStoreModel();
        UNIT_ASSERT_VALUES_EQUAL(model2.GetBlocksCount(), size2);
        UNIT_ASSERT_VALUES_EQUAL(model2.GetBlockSize(), DefaultBlockSize);
        UNIT_ASSERT_VALUES_EQUAL(model2.GetChannelsCount(), 19);

        auto& profile2 = model2.GetPerformanceProfile();
        UNIT_ASSERT_VALUES_EQUAL(profile2.GetMaxReadIops(), 300);
        UNIT_ASSERT_VALUES_EQUAL(profile2.GetMaxWriteIops(), 4800);
        UNIT_ASSERT_VALUES_EQUAL(profile2.GetMaxReadBandwidth(), 240_MB);
        UNIT_ASSERT_VALUES_EQUAL(profile2.GetMaxWriteBandwidth(), 240_MB);

        service.AssertDescribeFileStoreModelFailed(0);
        service.AssertDescribeFileStoreModelFailed(1000, 0);
    }

    Y_UNIT_TEST(ShouldCreateSession)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("test", 1000);

        auto headers = service.InitSession("test", "client");

        service.PingSession(headers);
        service.DestroySession(headers);
    }

    Y_UNIT_TEST(ShouldReturnFileStoreInfoWhenCreateSession)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("test", 1000, DefaultBlockSize, NProto::EStorageMediaKind::STORAGE_MEDIA_SSD);

        auto response = service.CreateSession(THeaders{"test", "client", ""});

        UNIT_ASSERT(response->Record.HasFileStore());
        UNIT_ASSERT_EQUAL(
            NProto::EStorageMediaKind::STORAGE_MEDIA_SSD,
            static_cast<NProto::EStorageMediaKind>(response->Record.GetFileStore().GetStorageMediaKind()));
    }

    Y_UNIT_TEST(ShouldRestoreSessionIfPipeFailed)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("test", 1000);

        auto& runtime = env.GetRuntime();

        bool fail = true;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                if (!fail) {
                    return TTestActorRuntime::DefaultObserverFunc(runtime, event);
                }

                switch (event->GetTypeRewrite()) {
                    case TEvIndexTablet::EvCreateSessionRequest: {
                        fail = false;
                        auto response = std::make_unique<TEvTabletPipe::TEvClientConnected>(
                            -1,
                            NKikimrProto::ERROR,
                            event->Sender,
                            event->Sender,
                            true,
                            false);

                        runtime.Send(
                            new IEventHandle(
                                // send back
                                event->Sender,
                                event->Sender,
                                response.release(),
                                0, // flags
                                event->Cookie),
                            nodeIdx);

                        return TTestActorRuntime::EEventAction::DROP;
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        THeaders headers = {"test", "client", ""};
        service.CreateSession(headers);
    }

    Y_UNIT_TEST(ShouldRestoreSessionIfCreateFailed)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("test", 1000);

        auto& runtime = env.GetRuntime();

        bool fail = true;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                if (!fail) {
                    return TTestActorRuntime::DefaultObserverFunc(runtime, event);
                }

                switch (event->GetTypeRewrite()) {
                    case TEvIndexTablet::EvCreateSessionRequest: {
                        fail = false;
                        auto response = std::make_unique<TEvIndexTablet::TEvCreateSessionResponse>(
                            MakeError(E_REJECTED, "xxx"));

                        runtime.Send(
                            new IEventHandle(
                                // send back
                                event->Sender,
                                event->Sender,
                                response.release(),
                                0, // flags
                                event->Cookie),
                            nodeIdx);

                        return TTestActorRuntime::EEventAction::DROP;
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        THeaders headers = {"test", "client", ""};
        service.AssertCreateSessionFailed(headers);
        service.CreateSession(headers);
    }

    Y_UNIT_TEST(ShouldFailIfCreateSessionFailed)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("test", 1000);

        auto& runtime = env.GetRuntime();

        bool fail = true;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                if (!fail) {
                    return TTestActorRuntime::DefaultObserverFunc(runtime, event);
                }

                switch (event->GetTypeRewrite()) {
                    case TEvIndexTablet::EvCreateSessionRequest: {
                        fail = false;
                        auto response = std::make_unique<TEvIndexTablet::TEvCreateSessionResponse>(
                            MakeError(E_REJECTED, "xxx"));

                        runtime.Send(
                            new IEventHandle(
                                // send back
                                event->Sender,
                                event->Sender,
                                response.release(),
                                0, // flags
                                event->Cookie),
                            nodeIdx);

                        return TTestActorRuntime::EEventAction::DROP;
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        THeaders headers = {"test", "client", ""};
        service.AssertCreateSessionFailed(headers);
        service.CreateSession(headers);
    }

    Y_UNIT_TEST(ShouldCleanUpIfSessionFailed)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        auto& runtime = env.GetRuntime();

        ui64 tabletId = -1;
        TActorId session;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) mutable {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvDescribeFileStoreResponse: {
                        TEvSSProxy::TEvDescribeFileStoreResponse::TPtr* ptr =
                            reinterpret_cast<typename TEvSSProxy::TEvDescribeFileStoreResponse::TPtr*>(
                                &event);

                        auto* msg = (*ptr)->Get();
                        const auto& desc = msg->PathDescription.GetFileStoreDescription();
                        tabletId = desc.GetIndexTabletId();

                        return TTestActorRuntime::EEventAction::PROCESS;
                    }
                    case TEvIndexTablet::EvCreateSessionRequest: {
                        session = event->Sender;
                        return TTestActorRuntime::EEventAction::PROCESS;
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("test", 1000);

        auto headers = service.InitSession("test", "client");
        UNIT_ASSERT(headers.SessionId);
        UNIT_ASSERT(tabletId != -1llu);
        UNIT_ASSERT(session);

        runtime.SetObserverFunc(TTestActorRuntime::DefaultObserverFunc);

        // explicitly fail session actor, proper way is to catch
        // IndexTablet::CreateSession request via observer func and respond w error
        // but for some reason runtime doesn't catch this event during tablet restart
        // though it actually happens and session resotres by the end of restart
        runtime.Send(
            new IEventHandle(
                // send back
                session,
                session,
                new TEvents::TEvPoisonPill(),
                0, // flags
                0),
            nodeIdx);

        TIndexTabletClient tablet(runtime, nodeIdx, tabletId);
        tablet.RebootTablet();

        auto response = service.AssertCreateNodeFailed(
            headers,
            TCreateNodeArgs::File(RootNodeId, "aaa"));

        UNIT_ASSERT_VALUES_EQUAL(response->GetError().GetCode(), (ui32)E_FS_INVALID_SESSION);

        service.CreateSession(headers);
        service.CreateNode(headers, TCreateNodeArgs::File(RootNodeId, "aaa"));
    }

    Y_UNIT_TEST(ShouldRestoreClientSession)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("test", 1000);

        auto headers = service.InitSession(
            "test",
            "client",
            "",         // checkpointId
            true        // restoreClientSession
        );
        UNIT_ASSERT_VALUES_UNEQUAL("", headers.SessionId);

        auto headers2 = service.InitSession(
            "test",
            "client",
            "",         // checkpointId
            true        // restoreClientSession
        );
        UNIT_ASSERT_VALUES_EQUAL(headers.SessionId, headers2.SessionId);

        auto headers3 = service.InitSession(
            "test",
            "client",
            "",         // checkpointId
            false       // restoreClientSession
        );
        UNIT_ASSERT_VALUES_UNEQUAL(headers.SessionId, headers3.SessionId);

        service.DestroySession(headers);
    }

    Y_UNIT_TEST(ShouldNotPingAndDestroyInvalidSession)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("test", 1000);

        auto headers = service.InitSession("test", "client");

        THeaders invalidFileSystem = {"xxx", "client", headers.SessionId};
        THeaders invalidClient = {"test", "invalid client", headers.SessionId};
        THeaders invalidSession = {"test", "client", "invalid session"};

        // FIXME
        // service.AssertPingSessionFailed(invalidFileSystem);
        service.AssertPingSessionFailed(invalidClient);
        service.AssertPingSessionFailed(invalidSession);

        service.AssertDestroySessionFailed(invalidFileSystem);
        service.AssertDestroySessionFailed(invalidClient);
        // fail safe
        service.DestroySession(invalidSession);
    }

    Y_UNIT_TEST(ShouldForwardRequests)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("test", 1000);

        auto headers = service.InitSession("test", "client");

        auto request = service.CreateCreateNodeRequest(
            headers,
            TCreateNodeArgs::File(RootNodeId, "file"));

        env.GetRuntime().DispatchEvents({}, TDuration::Seconds(1));
        service.SendCreateNodeRequest(std::move(request));

        auto response = service.RecvCreateNodeResponse();
        UNIT_ASSERT(response);
        UNIT_ASSERT_C(SUCCEEDED(response->GetStatus()), response->GetErrorReason().c_str());
    }

    Y_UNIT_TEST(ShouldNotForwardRequestsWithInvalidSession)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("test", 1000);

        auto headers = service.InitSession("test", "client");

        THeaders invalidFileSystem = {"xxx", "client", headers.SessionId};
        THeaders invalidClient = {"test", "invalid client", headers.SessionId};
        THeaders invalidSession = {"test", "client", "invalid session"};

        auto nodeArgs = TCreateNodeArgs::File(RootNodeId, "file");

        service.AssertCreateNodeFailed(invalidFileSystem, nodeArgs);
        service.AssertCreateNodeFailed(invalidClient, nodeArgs);
        service.AssertCreateNodeFailed(invalidSession, nodeArgs);

        // sanity check
        service.CreateNode(headers, nodeArgs);
    }

    Y_UNIT_TEST(ShouldGetSessionEvents)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("test", 1000);

        auto headers = service.InitSession("test", "client");

        service.SubscribeSession(headers);
        service.CreateNode(headers, TCreateNodeArgs::File(RootNodeId, "file"));

        auto response = service.GetSessionEvents(headers);

        const auto& events = response->Record.GetEvents();
        UNIT_ASSERT_VALUES_EQUAL(events.size(), 1);
    }

    Y_UNIT_TEST(ShouldGetSessionEventsStream)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("test", 1000);

        auto headers = service.InitSession("test", "client");

        service.SubscribeSession(headers);
        service.CreateNode(headers, TCreateNodeArgs::File(RootNodeId, "file1"));

        {
            auto response = service.GetSessionEventsStream(headers);

            const auto& events = response->Record.GetEvents();
            UNIT_ASSERT_VALUES_EQUAL(events.size(), 1);
        }

        service.CreateNode(headers, TCreateNodeArgs::File(RootNodeId, "file2"));

        {
            auto response = service.RecvResponse<TEvService::TEvGetSessionEventsResponse>();
            UNIT_ASSERT_C(
                SUCCEEDED(response->GetStatus()),
                response->GetErrorReason());

            const auto& events = response->Record.GetEvents();
            UNIT_ASSERT_VALUES_EQUAL(events.size(), 1);
        }
    }

    Y_UNIT_TEST(ShouldListFileStores)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        TServiceClient service(env.GetRuntime(), nodeIdx);

        TVector<TString> expected = {"dir/fs1", "dir/fs2", "dir1/fs", "dir2/fs"};
        for (const auto& id : expected) {
            service.CreateFileStore(id, 1000);
        }

        auto response = service.ListFileStores();
        const auto& proto = response->Record.GetFileStores();

        TVector<TString> filestores;
        Copy(proto.begin(), proto.end(), std::back_inserter(filestores));
        Sort(filestores);

        UNIT_ASSERT_VALUES_EQUAL(filestores, expected);
    }

    Y_UNIT_TEST(ShouldFailListFileStoresIfDescribeSchemeFails)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateFileStore("fs1", 10000);
        service.CreateFileStore("fs2", 10000);

        auto error = MakeError(E_ARGUMENT, "Error");

        auto& runtime = env.GetRuntime();
        runtime.SetObserverFunc(
            [=] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvDescribeSchemeRequest: {
                        auto response = std::make_unique<TEvSSProxy::TEvDescribeSchemeResponse>(
                            error);
                        runtime.Send(
                            new IEventHandle(
                                event->Sender,
                                event->Recipient,
                                response.release(),
                                0, // flags
                                event->Cookie),
                            nodeIdx);
                        return TTestActorRuntime::EEventAction::DROP;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        auto response = service.AssertListFileStoresFailed();
        UNIT_ASSERT(response->GetStatus() == error.GetCode());
        UNIT_ASSERT(response->GetErrorReason() == error.GetMessage());
    }
}

}   // namespace NCloud::NFileStore::NStorage
