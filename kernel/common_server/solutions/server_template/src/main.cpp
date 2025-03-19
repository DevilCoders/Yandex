#include <kernel/daemon/daemon.h>
#include "server/server.h"
#include <kernel/common_server/api/history/config.h>
#include <kernel/common_server/tags/abstract.h>
#include <mapreduce/yt/interface/init.h>

int main(int argc, char* argv[]) {
    NYT::Initialize(argc, argv);
    THistoryConfig::DefaultDeep = TDuration::Minutes(10);
    THistoryConfig::DefaultMaxHistoryDeep = TDuration::Minutes(10);
    THistoryConfig::DefaultEventsStoreInMemDeep = TDuration::Minutes(10);
    TTagStorageCustomization::WriteOldBinaryData = false;
    TTagStorageCustomization::ReadOldBinaryData = false;
    TTagStorageCustomization::WriteNewBinaryData = true;
    TTagStorageCustomization::ReadNewBinaryData = true;
    return Singleton<TDaemon<NCS::NServerTemplate::TServer> >()->Run(argc, argv);
}

