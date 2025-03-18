#pragma once

#include "json_to_proto_config.h"

#include <util/generic/vector.h>

#include <google/protobuf/message.h>
#include <google/protobuf/stubs/common.h>

#include <library/cpp/protobuf/json/json2proto.h>

namespace NProtoConfig {
    struct TLoadConfigOptions {
        enum class EConfigFormat {
            Json      = 0 /* "Json" */,
            ProtoText = 1 /* "ProtoText" */,
        };
        TString Resource;
        TString Path;
        TVector<TString> Overrides;
        EConfigFormat ConfigFormat = EConfigFormat::Json;
        NProtobufJson::TJson2ProtoConfig Json2ProtoOptions = JSON_2_PROTO_CONFIG;
    };

    void GetOpt(int argc, const char* argv[], NProtoBuf::Message& config, TLoadConfigOptions& options);
    void GetOpt(int argc, const char* argv[], NProtoBuf::Message& config, const TString& resource);

    template <typename TProtoConfig>
    TProtoConfig GetOpt(int argc, const char* argv[], const TString& resource = "") {
        TProtoConfig config;
        GetOpt(argc, argv, config, resource);
        return config;
    }

    void LoadWithOptions(NProtoBuf::Message& config, const TLoadConfigOptions& options);

    template <typename TProtoConfig>
    TProtoConfig LoadWithOptions(const TLoadConfigOptions& options) {
        TProtoConfig config;
        LoadWithOptions(config, options);
        return config;
    }

}
