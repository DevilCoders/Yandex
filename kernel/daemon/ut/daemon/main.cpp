#include "server.h"
#include <kernel/daemon/daemon.h>
#include <kernel/searchlog/errorlog.h>

int main(int argc, char* argv[]) {
    Singleton<TErrorLog>()->OpenLog("stderr_search");
    return Singleton<TDaemon<TServer> >()->Run(argc, argv);
}

