#include <library/cpp/proto_config/exec_test/tool/config.pb.h>

#include <library/cpp/proto_config/load.h>

int main(int argc, const char* argv[]) {
    NProtoConfig::GetOpt<TConfig>(argc, argv);
    return 0;
}
