#include "ss_proxy.h"

#include <cloud/filestore/libs/storage/testlib/ss_proxy_client.h>
#include <cloud/filestore/libs/storage/testlib/test_env.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

NKikimrSchemeOp::TModifyScheme CreateDir(const TString& path)
{
    TStringBuf workingDir;
    TStringBuf name;
    TStringBuf(path).RSplit('/', workingDir, name);

    NKikimrSchemeOp::TModifyScheme modifyScheme;
    modifyScheme.SetOperationType(NKikimrSchemeOp::ESchemeOpMkDir);
    modifyScheme.SetWorkingDir(TString(workingDir));

    auto* op = modifyScheme.MutableMkDir();
    op->SetName(TString(name));

    return modifyScheme;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TSSProxyTest)
{
    Y_UNIT_TEST(ShouldCreateDirectories)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TSSProxyClient ssProxy(env.GetStorageConfig(), env.GetRuntime(), nodeIdx);
        ssProxy.ModifyScheme(CreateDir("/local/foo"));
        ssProxy.ModifyScheme(CreateDir("/local/bar"));

        ssProxy.DescribeScheme("/local/foo");
        ssProxy.DescribeScheme("/local/bar");
    }

    Y_UNIT_TEST(ShouldCreateFileStore)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TSSProxyClient ssProxy(env.GetStorageConfig(), env.GetRuntime(), nodeIdx);
        ssProxy.CreateFileStore("test1", 1000);
        ssProxy.CreateFileStore("test2", 2000);

        ssProxy.DescribeFileStore("test1");
        ssProxy.DescribeFileStore("test2");

        ssProxy.DestroyFileStore("test1");
        ssProxy.DestroyFileStore("test2");
    }

    Y_UNIT_TEST(ShouldDestroyNonExistentFilestore)
    {
        TTestEnv env;
        env.CreateSubDomain("nfs");

        ui32 nodeIdx = env.CreateNode("nfs");

        TSSProxyClient ssProxy(env.GetStorageConfig(), env.GetRuntime(), nodeIdx);
        auto response = ssProxy.DestroyFileStore("nonexistent");
        UNIT_ASSERT_VALUES_EQUAL(response->GetError().GetCode(), S_FALSE);
    }
}

}   // namespace NCloud::NFileStore::NStorage
