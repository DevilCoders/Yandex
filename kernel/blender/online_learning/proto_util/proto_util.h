#pragma once

#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>

#include <util/generic/strbuf.h>

namespace NBlenderOnlineLearning {
    TString OutProto(const ::google::protobuf::Message& proto, bool debug = false);
    bool LoadProto(::google::protobuf::Message& result, const TStringBuf& serialized,  bool debug = false);
}
