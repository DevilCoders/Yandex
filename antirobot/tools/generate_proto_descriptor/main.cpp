#include <antirobot/idl/daemon_log.pb.h>

#include <library/cpp/protobuf/yql/descriptor.h>

#include <util/stream/output.h>

template<typename T>
TString GetMeta() {
    TString config = GenerateProtobufTypeConfig<T>();
    auto typeConfig = ParseTypeConfig(config);
    return typeConfig.Metadata;
}

int main() {
    Cerr << "\"antirobot-daemon-cacher-log\": \"" << GetMeta<NDaemonLog::TCacherRecord>() << "\",\n";
    Cerr << "\"antirobot-daemon-processor-log\": \"" << GetMeta<NDaemonLog::TProcessorRecord>() << "\",\n";
}
