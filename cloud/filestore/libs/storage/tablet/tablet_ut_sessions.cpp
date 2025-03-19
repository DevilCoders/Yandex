#include "tablet.h"

#include <cloud/filestore/libs/storage/testlib/tablet_client.h>
#include <cloud/filestore/libs/storage/testlib/test_env.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/size_literals.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TIndexTabletTest_Sessions)
{
    Y_UNIT_TEST(ShouldTrackClients)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);

        tablet.CreateSession("client", "session");
        tablet.DestroySession("client", "session");
    }

    Y_UNIT_TEST(ShouldTrackHandles)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);

        tablet.InitSession("client", "session");

        ui64 id = CreateNode(tablet, TCreateNodeArgs::File(RootNodeId, "test"));

        ui64 handle = CreateHandle(tablet, id);

        tablet.DestroyHandle(handle);
    }

    Y_UNIT_TEST(ShouldTrackLocks)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);

        tablet.InitSession("client", "session");

        auto id = CreateNode(tablet, TCreateNodeArgs::File(RootNodeId, "test"));
        ui64 handle = CreateHandle(tablet, id);

        tablet.AcquireLock(handle, 1, 0, 4_KB);

        auto response = tablet.AssertAcquireLockFailed(handle, 2, 0, 4_KB);
        UNIT_ASSERT_VALUES_EQUAL(response->GetError().GetCode(), E_FS_WOULDBLOCK);

        tablet.ReleaseLock(handle, 1, 0, 4_KB);
        tablet.AcquireLock(handle, 2, 0, 4_KB);

        response = tablet.AssertAcquireLockFailed(handle, 1, 0, 0);
        UNIT_ASSERT_VALUES_EQUAL(response->GetError().GetCode(), E_FS_WOULDBLOCK);

        tablet.ReleaseLock(handle, 2, 0, 4_KB);
        tablet.AcquireLock(handle, 1, 0, 0);

        response = tablet.AssertAcquireLockFailed(handle, 2, 0, 4_KB);
        UNIT_ASSERT_VALUES_EQUAL(response->GetError().GetCode(), E_FS_WOULDBLOCK);

        tablet.RebootTablet();
        tablet.InitSession("client", "session");

        response = tablet.AssertAcquireLockFailed(handle, 2, 0, 4_KB);
        UNIT_ASSERT_VALUES_EQUAL(response->GetError().GetCode(), E_FS_WOULDBLOCK);

        tablet.DestroyHandle(handle);

        // FIXME: NBS-2933 for exclusive lock you should have write acces
        /*
        handle = CreateHandle(tablet, id, {}, TCreateHandleArgs::RDNLY);
        tablet.AssertTestLockFailed(handle, 1, 0, 4_KB);
        tablet.AssertAcquireLockFailed(handle, 1, 0, 4_KB);
        */
    }

    Y_UNIT_TEST(ShouldTrackSharedLocks)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);

        tablet.InitSession("client", "session");

        auto id = CreateNode(tablet, TCreateNodeArgs::File(RootNodeId, "test"));

        ui64 handle = CreateHandle(tablet, id);

        tablet.AcquireLock(handle, 1, 0, 4_KB, NProto::E_SHARED);
        tablet.AcquireLock(handle, 2, 0, 4_KB, NProto::E_SHARED);

        auto response = tablet.AssertAcquireLockFailed(handle, 1, 0, 0);
        UNIT_ASSERT_VALUES_EQUAL(response->GetError().GetCode(), E_FS_WOULDBLOCK);

        tablet.RebootTablet();
        tablet.InitSession("client", "session");

        response = tablet.AssertAcquireLockFailed(handle, 1, 0, 0);
        UNIT_ASSERT_VALUES_EQUAL(response->GetError().GetCode(), E_FS_WOULDBLOCK);

        tablet.AcquireLock(handle, 3, 0, 4_KB, NProto::E_SHARED);

        tablet.DestroyHandle(handle);

        // FIXME: NBS-2933 for shared lock you should have read acces
        /*
        handle = CreateHandle(tablet, id, {}, TCreateHandleArgs::WRNLY);
        tablet.AssertTestLockFailed(handle, 1, 0, 4_KB, NProto::E_SHARED);
        tablet.AssertAcquireLockFailed(handle, 1, 0, 4_KB, NProto::E_SHARED);
        */
    }

    Y_UNIT_TEST(ShouldCleanupLocksKeptBySession)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);

        tablet.InitSession("client", "session");

        auto id1 = CreateNode(tablet, TCreateNodeArgs::File(RootNodeId, "test1"));
        auto id2 = CreateNode(tablet, TCreateNodeArgs::File(RootNodeId, "test2"));

        ui64 handle1 = CreateHandle(tablet, id1);
        ui64 handle2 = CreateHandle(tablet, id2);

        tablet.AcquireLock(handle1, 1, 0, 0);
        tablet.AcquireLock(handle2, 1, 0, 4_KB);

        tablet.RebootTablet();
        tablet.DestroySession("client", "session");
        tablet.RebootTablet();

        tablet.InitSession("client", "session2");
        handle1 = CreateHandle(tablet, id1);
        tablet.AcquireLock(handle1, 1, 0, 4_KB);

        auto response = tablet.AssertAcquireLockFailed(handle1, 2, 0, 4_KB);
        UNIT_ASSERT_VALUES_EQUAL(response->GetError().GetCode(), E_FS_WOULDBLOCK);
    }

    Y_UNIT_TEST(ShouldCreateHandlesAndFiles)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);
        tablet.InitSession("client", "session");

        auto id1 = CreateNode(tablet, TCreateNodeArgs::File(RootNodeId, "test"));

        tablet.CreateHandle(id1, TCreateHandleArgs::CREATE);
        tablet.CreateHandle(id1, TCreateHandleArgs::RDWR);

        auto response2 = tablet.CreateHandle(RootNodeId, "xxx", TCreateHandleArgs::CREATE);
        auto id2 = response2->Record.GetNodeAttr().GetId();
        UNIT_ASSERT(id2 != InvalidNodeId);

        auto response3 = tablet.CreateHandle(RootNodeId, "yyy", TCreateHandleArgs::CREATE_EXL);
        auto id3 = response3->Record.GetNodeAttr().GetId();
        UNIT_ASSERT(id3 != InvalidNodeId);

        auto response = tablet.ListNodes(RootNodeId);
        const auto& names = response->Record.GetNames();
        UNIT_ASSERT_VALUES_EQUAL(names.size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(names[0], "test");
        UNIT_ASSERT_VALUES_EQUAL(names[1], "xxx");
        UNIT_ASSERT_VALUES_EQUAL(names[2], "yyy");

        const auto& nodes = response->Record.GetNodes();
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(nodes.Get(0).GetId(), id1);
        UNIT_ASSERT_VALUES_EQUAL(nodes.Get(1).GetId(), id2);
        UNIT_ASSERT_VALUES_EQUAL(nodes.Get(2).GetId(), id3);
        UNIT_ASSERT_VALUES_EQUAL(nodes.Get(2).GetLinks(), 1);
    }

    Y_UNIT_TEST(ShouldNotCreateInvalidHandles)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);
        tablet.InitSession("client", "session");

        auto id = CreateNode(tablet, TCreateNodeArgs::File(RootNodeId, "test"));

        tablet.AssertCreateHandleFailed(100500, TCreateHandleArgs::RDWR);
        // create file under file node
        tablet.AssertCreateHandleFailed(id, "xxx", TCreateHandleArgs::CREATE);
        // create the same file with O_EXCL
        tablet.AssertCreateHandleFailed(RootNodeId, "test", TCreateHandleArgs::CREATE_EXL);
        // open non existent file w/o O_CREAT
        tablet.AssertCreateHandleFailed(RootNodeId, "xxx", TCreateHandleArgs::RDWR);
    }

    Y_UNIT_TEST(ShouldKeepFileByOpenHandle)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);
        tablet.InitSession("client", "session");

        auto id = CreateNode(tablet, TCreateNodeArgs::File(RootNodeId, "test"));
        auto handle = CreateHandle(tablet, id);

        tablet.UnlinkNode(RootNodeId, "test", false);

        auto attrs = GetNodeAttrs(tablet, id);
        UNIT_ASSERT_VALUES_EQUAL(attrs.GetId(), id);
        UNIT_ASSERT_VALUES_EQUAL(attrs.GetSize(), 0);

        tablet.WriteData(handle, 0, 1_KB, '1');
        UNIT_ASSERT_VALUES_EQUAL(GetNodeAttrs(tablet, id).GetSize(), 1_KB);

        auto buffer = ReadData(tablet, handle, 16_KB, 0);
        UNIT_ASSERT_VALUES_EQUAL(buffer.size(), 1_KB);
        auto expected = TString(1_KB, '1');
        UNIT_ASSERT_VALUES_EQUAL(buffer, expected);

        tablet.DestroyHandle(handle);
        tablet.AssertGetNodeAttrFailed(id);
        tablet.AssertWriteDataFailed(handle, 0, 1_KB, '2');
        tablet.AssertReadDataFailed(handle, 16_KB, 0);
    }

    Y_UNIT_TEST(ShouldCleanupHandlesAndNodesKeptBySession)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);
        tablet.InitSession("client", "session");

        auto id = CreateNode(tablet, TCreateNodeArgs::File(RootNodeId, "test"));
        auto handle = CreateHandle(tablet, id);

        tablet.UnlinkNode(RootNodeId, "test", false);
        tablet.WriteData(handle, 0, 1_KB, '1');

        tablet.DestroySession("client", "session");

        tablet.InitSession("client", "session");

        tablet.AssertGetNodeAttrFailed(id);
        tablet.AssertWriteDataFailed(handle, 0, 1_KB, '2');
        tablet.AssertReadDataFailed(handle, 0, 1_KB);
    }

    Y_UNIT_TEST(ShouldSendSessionEvents)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);
        tablet.InitSession("client", "session");

        tablet.SubscribeSession();
        tablet.CreateNode(TCreateNodeArgs::File(RootNodeId, "test"));

        auto response = tablet.RecvResponse<TEvService::TEvGetSessionEventsResponse>();
        UNIT_ASSERT_C(
            SUCCEEDED(response->GetStatus()),
            response->GetErrorReason());

        const auto& events = response->Record.GetEvents();
        UNIT_ASSERT_VALUES_EQUAL(events.size(), 1);
    }

    Y_UNIT_TEST(ShouldRestoreClientSession)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);

        TString sessionId;
        {
            auto response = tablet.CreateSession(
                "client",
                "session",  // sessionId
                "",         // checkpointId
                true        // restoreClientSession
            );
            sessionId = response->Record.GetSessionId();
        }
        UNIT_ASSERT_VALUES_EQUAL("session", sessionId);

        tablet.SetHeaders("client", sessionId);

        ui64 id = CreateNode(tablet, TCreateNodeArgs::File(RootNodeId, "test"));

        ui64 handle = CreateHandle(tablet, id);
        tablet.WriteData(handle, 0, 4_KB, '0');

        {
            auto response = tablet.CreateSession(
                "client",
                "vasya",    // sessionId
                "",         // checkpointId
                true        // restoreClientSession
            );
            UNIT_ASSERT_VALUES_EQUAL("session", response->Record.GetSessionId());
        }

        {
            auto response = tablet.CreateSession(
                "client",
                "",         // sessionId
                "",         // checkpointId
                true        // restoreClientSession
            );
            UNIT_ASSERT_VALUES_EQUAL("session", response->Record.GetSessionId());
        }

        TString expected;
        expected.ReserveAndResize(4_KB);
        memset(expected.begin(), '0', 4_KB);

        {
            auto response = tablet.ReadData(handle, 0, 4_KB);
            const auto& buffer = response->Record.GetBuffer();
            UNIT_ASSERT_VALUES_EQUAL(expected, buffer);
        }
    }

    Y_UNIT_TEST(ShouldResetSessionState)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);
        tablet.InitSession("client", "session");

        auto id1 = CreateNode(tablet, TCreateNodeArgs::File(RootNodeId, "test"));
        auto handle1 = CreateHandle(tablet, id1);
        tablet.AcquireLock(handle1, 1, 0, 1_KB);

        auto id2 = CreateNode(tablet, TCreateNodeArgs::File(RootNodeId, "xxx"));
        CreateHandle(tablet, id2);
        tablet.UnlinkNode(RootNodeId, "xxx", false);

        tablet.ResetSession("client", "session", "state");

        // check that handles are invalidated
        auto lock = tablet.AssertAcquireLockFailed(handle1, 1, 0, 1_KB);
        UNIT_ASSERT_VALUES_EQUAL(lock->Record.GetError().GetCode(), (ui32)E_FS_BADHANDLE);

        // check that locks are invalidated
        handle1 = CreateHandle(tablet, id1);
        tablet.AcquireLock(handle1, 2, 0, 1_KB);

        // check that nodes are cleaned up
        tablet.AssertAccessNodeFailed(id2);

        // check that state is properly returned
        auto create = tablet.CreateSession("client", "session");
        UNIT_ASSERT_VALUES_EQUAL(create->Record.GetSessionState(), "state");

        tablet.ResetSession("client", "session", "");

        // check that state is properly updated
        create = tablet.CreateSession("client", "session");
        UNIT_ASSERT_VALUES_EQUAL(create->Record.GetSessionState(), "");
    }

    Y_UNIT_TEST(ShouldCleanupHandlesAndLocksKeptBySession)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);
        tablet.InitSession("client", "session");

        auto id = CreateNode(tablet, TCreateNodeArgs::File(RootNodeId, "test"));
        auto handle1 = CreateHandle(tablet, id);
        auto handle2 = CreateHandle(tablet, id);

        tablet.AcquireLock(handle1, 1, 0, 1_KB);
        tablet.AcquireLock(handle1, 1, 1_KB, 1_KB);

        tablet.AssertAcquireLockFailed(handle2, 2, 0, 1_KB);
        tablet.DestroyHandle(handle1);

        tablet.AcquireLock(handle2, 1, 0, 1_KB);
        tablet.AcquireLock(handle2, 1, 1_KB, 1_KB);

        tablet.DestroySession("client", "session");
        tablet.InitSession("client", "session");

        auto handle3 = CreateHandle(tablet, id);
        tablet.AcquireLock(handle3, 1, 0, 1_KB);
        tablet.AcquireLock(handle3, 1, 1_KB, 1_KB);
    }

    Y_UNIT_TEST(ShouldDeduplicateCreateHandleRequests)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);
        tablet.InitSession("client", "session");

        auto createRequest = [&] (ui64 reqId) {
            auto request = tablet.CreateCreateHandleRequest(
                RootNodeId,
                "xxx",
                TCreateHandleArgs::CREATE_EXL);
            request->Record.MutableHeaders()->SetRequestId(reqId);

            return std::move(request);
        };

        ui64 handle = 0;
        tablet.SendRequest(createRequest(100500));
        {
            auto response = tablet.RecvCreateHandleResponse();
            UNIT_ASSERT(!HasError(response->Record.GetError()));

            handle = response->Record.GetHandle();
            UNIT_ASSERT(handle);
        }

        tablet.SendRequest(createRequest(100500));
        {
            auto response = tablet.RecvCreateHandleResponse();
            UNIT_ASSERT(!HasError(response->GetError()));

            UNIT_ASSERT_VALUES_EQUAL(handle, response->Record.GetHandle());
        }

        tablet.RebootTablet();
        tablet.InitSession("client", "session");

        tablet.SendRequest(createRequest(100500));
        {
            auto response = tablet.RecvCreateHandleResponse();
            UNIT_ASSERT(!HasError(response->GetError()));
            UNIT_ASSERT_VALUES_EQUAL(handle, response->Record.GetHandle());
        }

        tablet.SendRequest(createRequest(100501));
        {
            auto response = tablet.RecvCreateHandleResponse();
            UNIT_ASSERT(HasError(response->GetError()));
        }
    }

    Y_UNIT_TEST(ShouldCleanupDedupCacheKeptBySession)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");
        ui64 tabletId = env.BootIndexTablet(nodeIdx);

        TIndexTabletClient tablet(env.GetRuntime(), nodeIdx, tabletId);
        tablet.InitSession("client", "session");

        auto createRequest = [&] (ui64 reqId, const TString& name) {
            auto request = tablet.CreateCreateHandleRequest(
                RootNodeId,
                name,
                TCreateHandleArgs::CREATE_EXL);
            request->Record.MutableHeaders()->SetRequestId(reqId);

            return std::move(request);
        };

        tablet.SendRequest(createRequest(100500, "xxx"));
        {
            auto response = tablet.RecvCreateHandleResponse();
            UNIT_ASSERT(!HasError(response->Record.GetError()));
        }

        tablet.SendRequest(createRequest(100501, "yyy"));
        {
            auto response = tablet.RecvCreateHandleResponse();
            UNIT_ASSERT(!HasError(response->GetError()));
        }

        tablet.SendRequest(createRequest(100500, "xxx"));
        {
            auto response = tablet.RecvCreateHandleResponse();
            UNIT_ASSERT(!HasError(response->Record.GetError()));
        }

        tablet.SendRequest(createRequest(100501, "yyy"));
        {
            auto response = tablet.RecvCreateHandleResponse();
            UNIT_ASSERT(!HasError(response->GetError()));
        }

        tablet.DestroySession("client", "session");
        tablet.InitSession("client", "session");

        tablet.SendRequest(createRequest(100500, "xxx"));
        {
            auto response = tablet.RecvCreateHandleResponse();
            UNIT_ASSERT(HasError(response->Record.GetError()));
        }

        tablet.SendRequest(createRequest(100501, "yyy"));
        {
            auto response = tablet.RecvCreateHandleResponse();
            UNIT_ASSERT(HasError(response->GetError()));
        }
    }
}

}   // namespace NCloud::NFileStore::NStorage
