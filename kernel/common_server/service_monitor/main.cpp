#include <kernel/common_server/service_monitor/server/server.h>

#include <kernel/daemon/daemon.h>

int main(int argc, char* argv[]) {
    return Singleton<TDaemon<NServiceMonitor::TServer>>()->Run(argc, argv);
}
