// Tool for debuging begemot mapper failures. Expects json with AppHost context on stdin.

#include <search/begemot/server/server.h>
#include <search/begemot/server/worker.h>
#include <apphost/lib/service_testing/service_testing.h>


int main() {
    TString shardPath("../../../../search/begemot/data/Merger/search/wizard/data/wizard");
    NBg::NProto::TConfig config;
    config.SetEventLog("/dev/null");
    config.SetThreads(1);
    config.SetPrintRrrList(true);
    config.SetPrintRuleGraph(true);
    config.SetDataDir(shardPath);

    auto fs = NBg::CreateFileSystem(config);
    NBg::TWorker worker(NBg::DefaultRuleFactory(), *fs, config, NBg::CreateEventLog(config.GetEventLog(), 0), NBg::CreateEventLog(""), NBg::CreateEventLog(""), NBg::CreateEventLog(""));

    NAppHost::NService::TTestContext ctx(&Cin);
    worker(ctx, nullptr, nullptr);
    Cout << ToString(ctx.GetResult()) << Endl;

    return 0;
}
