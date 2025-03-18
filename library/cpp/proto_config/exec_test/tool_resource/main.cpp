#include <library/cpp/proto_config/exec_test/tool_resource/config.pb.h>
#include <library/cpp/proto_config/load.h>

#include <util/stream/output.h>

int main(int argc, const char* argv[]) {
    const TConfig cfg = NProtoConfig::GetOpt<TConfig>(argc, argv, "/config/config_resource.json");
    Cout << cfg.DebugString() << Endl;
    return 0;
}
