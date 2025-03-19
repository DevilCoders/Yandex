#include "proto_util.h"
#include <util/string/cast.h>

TString NBlenderOnlineLearning::OutProto(const ::google::protobuf::Message& proto, bool debug) {
    if (debug) {
        return proto.ShortDebugString();
    }
    return proto.SerializeAsString();
}

bool NBlenderOnlineLearning::LoadProto(::google::protobuf::Message& result, const TStringBuf& serialized,  bool debug) {
    if (debug) {
        //::google::protobuf::LogSilencer logSilencer;
        return ::google::protobuf::TextFormat::ParseFromString(ToString(serialized), &result);
    }
    return result.ParseFromArray(serialized.data(), static_cast<int>(serialized.size()));
}
