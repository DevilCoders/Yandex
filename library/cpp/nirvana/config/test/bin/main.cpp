#include <library/cpp/nirvana/config/test/bin/config.sc.h>

#include <library/cpp/nirvana/config/config.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/output.h>

int main(int argc, const char** argv) {
    NLastGetopt::TOpts parser;
    NNirvana::TConfigOption<NTestConfig::TConfig> config(parser);
    NLastGetopt::TOptsParseResult options(&parser, argc, argv);

    auto cfg = config.Config();
    Cout << "cluster " << cfg.YtCluster() << Endl;
    Cout << "int " << cfg.MyInt() << Endl;
    Cout << "str " << cfg.MyStr() << Endl;
    if (cfg.HasMyArr()) {
        Cout << "arr[1] " << cfg.MyArr(1) << Endl;
    }
    return 0;
}
