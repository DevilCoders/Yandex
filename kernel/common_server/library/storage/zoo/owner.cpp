#include "owner.h"
#include <library/cpp/logger/global/global.h>

namespace NRTProc {

    bool TZooOwner::BuildZKImpl(const TString& reason, const TString& root) const {
        TWriteGuard wg(RWMutex);
        WARNING_LOG << "ZK rebuild on " << reason << " for " << root << " starting..." << Endl;
        for (ui32 i = 0; i < 100; ++i) {
            NZooKeeper::TZooKeeperOptions options(SelfOptions.Address + TFsPath("/" + root).Fix().GetPath());
            options.Timeout = TDuration::MilliSeconds(10000000);
            options.LogLevel = static_cast<NZooKeeper::ELogLevel>(SelfOptions.LogLevel);
            ZK = MakeHolder<NZooKeeper::TZooKeeper>(options);
            TVector<TString> nodes;
            try {
                ZK->Exists("/");
                INFO_LOG << "ZK rebuild finished" << Endl;
                return true;
            } catch (...) {
                WARNING_LOG << "Can't works with zookeeper : " << CurrentExceptionMessage() << Endl;
            }
            Sleep(TDuration::MilliSeconds(500));
        }
        FAIL_LOG("Can't use ZooKeeper %s for root = %s", SelfOptions.Address.data(), SelfOptions.GetRoot().data());
        return false;
    }

    bool TZooOwner::BuildZK(const TString& reason) const {
        WARNING_LOG << "ZK rebuild on " << reason << " starting..." << Endl;
        if (AtFirstBuild) {
            BuildZKImpl(reason, "/");

            Y_VERIFY(SelfOptions.GetRoot().StartsWith('/'), "path should be absolute");
            try {
                TFsPath fs(SelfOptions.GetRoot());
                TReadGuard wg(RWMutex);
                ZK->CreateHierarchy(fs, NZooKeeper::ACLS_ALL);
            } catch (NZooKeeper::TInvalidStateError&) {
                return BuildZK("MkDirs failed: " + CurrentExceptionMessage());
            } catch (NZooKeeper::TConnectionLostError&) {
                return BuildZK("MkDirs failed: " + CurrentExceptionMessage());
            }

            AtFirstBuild = false;
        }
        return BuildZKImpl(reason, SelfOptions.GetRoot());
    }

    TZooOwner::TZooOwner(const TZooStorageOptions& selfOptions) : SelfOptions(selfOptions) {
        BuildZK("starting");
    }

}
