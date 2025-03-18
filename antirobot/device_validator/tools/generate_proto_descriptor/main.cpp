#include <antirobot/device_validator/proto/log.pb.h>

#include <library/cpp/protobuf/yql/descriptor.h>

#include <util/stream/output.h>


int main() {
    Cerr << GenerateProtobufTypeConfig<NDeviceValidatorProto::TLogRecord>() << Endl;
}
