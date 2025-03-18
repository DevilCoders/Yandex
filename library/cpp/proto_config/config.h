#pragma once

#include "json_to_proto_config.h"

#include <library/cpp/config/sax.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/generic/string.h>
#include <util/generic/variant.h>
#include <util/stream/file.h>

#include <google/protobuf/message.h>

namespace NProtoConfig {
    void ParseConfigFromJson(TStringBuf conf, NProtoBuf::Message& message, const NProtobufJson::TJson2ProtoConfig proto2JsonConfig = JSON_2_PROTO_CONFIG);
    void LoadConfigFromResource(const TString& path, NProtoBuf::Message& message);
    void LoadConfigFromJsonFile(const TString& path, NProtoBuf::Message& message);

    template <typename TMessage>
    TMessage ParseConfigFromJson(TStringBuf conf) {
        TMessage message;
        ParseConfigFromJson(conf, message);
        return message;
    }

    template <typename TMessage>
    TMessage LoadConfigFromJsonFile(const TString& path) {
        TMessage message;
        LoadConfigFromJsonFile(path, message);
        return message;
    }

    /**
    Overrides proto message leaf's value
    $with should be in form of 'key1.key2.key3.leaf=value'
    some proto types not supported - may throw yexception
    TODO(velavokr): Does not support repeated and map fields */
    void OverrideConfig(google::protobuf::Message& config, const TString& with);



    template <typename T>
    using TIsProtoMessage = std::enable_if_t<std::is_base_of<NProtoBuf::Message, T>::value, bool>;

    template <typename T>
    using TIsNotProtoMessage = std::enable_if_t<!std::is_base_of<NProtoBuf::Message, T>::value, bool>;



    void ParseConfigFromJson(IInputStream& input, NProtoBuf::Message& message, const NProtobufJson::TJson2ProtoConfig proto2JsonConfig = JSON_2_PROTO_CONFIG);

    template <typename T, TIsProtoMessage<T> = true>
    inline void ParseConfigFromJson(IInputStream& input, T& message, const NProtobufJson::TJson2ProtoConfig proto2JsonConfig = JSON_2_PROTO_CONFIG) {
        ParseConfigFromJson(input, static_cast<NProtoBuf::Message&>(message), proto2JsonConfig);
    }

    template <typename T, TIsNotProtoMessage<T> = true>
    inline void ParseConfigFromJson(IInputStream& input, T& config, const NProtobufJson::TJson2ProtoConfig proto2JsonConfig = JSON_2_PROTO_CONFIG) {
        typename T::TProto protobuf;
        ParseConfigFromJson(input, protobuf, proto2JsonConfig);
        config = T(protobuf);
    }

    template <typename T>
    T ParseConfigFromJson(IInputStream& input, const NProtobufJson::TJson2ProtoConfig proto2JsonConfig = JSON_2_PROTO_CONFIG) {
        T res;
        ParseConfigFromJson(input, res, proto2JsonConfig);
        return res;
    }



    template <typename T, TIsProtoMessage<T> = true>
    inline void ParseConfigFromTextFormat(IInputStream& input, T& message) {
        ::ParseFromTextFormat(input, message);
    }

    template <typename T, TIsNotProtoMessage<T> = true>
    void ParseConfigFromTextFormat(IInputStream& input, T& config) {
        typename T::TProto protobuf;
        ParseConfigFromTextFormat(input, protobuf);
        config = T(protobuf);
    }

    template <typename T>
    T ParseConfigFromTextFormat(IInputStream& input) {
        T res;
        ParseConfigFromTextFormat(input, res);
        return res;
    }

    struct TField {
        TString Name;

    public:
        explicit TField(TString str)
            : Name(std::move(str))
        {}
    };

    struct TMapIdx {
        TString Idx;

    public:
        explicit TMapIdx(TString str)
            : Idx(std::move(str))
        {}
    };

    struct TIdx {
        size_t Idx = -1;

    public:
        explicit TIdx(size_t idx)
            : Idx(idx)
        {}
    };

    using TKey = std::variant<TField, TMapIdx, TIdx>;
    using TKeyStack = TVector<TKey>;

    using TUnknownFieldCb = std::function<void(const TString& key, NConfig::IConfig::IValue* value)>;
    using TStackUnknownFieldCb = std::function<void(const TKeyStack& ctx, const TString& key, NConfig::IConfig::IValue* value)>;
    using TUnknownFieldCbImpl = std::variant<std::monostate, TUnknownFieldCb, TStackUnknownFieldCb>;

    void ParseConfigImpl(NConfig::IConfig& config, NProtoBuf::Message& message, TUnknownFieldCbImpl uf);

    template <typename T, TIsProtoMessage<T> = true>
    void ParseConfigImpl(NConfig::IConfig& iconfig, T& config, TUnknownFieldCbImpl uf) {
        ParseConfigImpl(iconfig, static_cast<NProtoBuf::Message&>(config), uf);
    }

    template <typename T, TIsNotProtoMessage<T> = true>
    void ParseConfigImpl(NConfig::IConfig& iconfig, T& config, TUnknownFieldCbImpl uf) {
        typename T::TProto protobuf;
        ParseConfigImpl(iconfig, protobuf, uf);
        config = T(protobuf);
    }

    template <typename T>
    auto ParseConfigImpl(NConfig::IConfig& iconfig, TUnknownFieldCbImpl uf) {
        T res;
        ParseConfigImpl(iconfig, res, uf);
        return res;
    }

    template <typename T>
    void ParseConfig(NConfig::IConfig& iconfig, T& message) {
        ParseConfigImpl(iconfig, message, {});
    }

    template <typename T>
    void ParseConfig(NConfig::IConfig& iconfig, T& message, TUnknownFieldCb uf) {
        ParseConfigImpl(iconfig, message, uf);
    }

    template <typename T>
    void ParseConfig(NConfig::IConfig& iconfig, T& message, TStackUnknownFieldCb uf) {
        ParseConfigImpl(iconfig, message, uf);
    }

    template <typename T>
    auto ParseConfig(NConfig::IConfig& iconfig) {
        return ParseConfigImpl<T>(iconfig, {});
    }

    template <typename T>
    auto ParseConfig(NConfig::IConfig& iconfig, TUnknownFieldCb uf) {
        return ParseConfigImpl<T>(iconfig, uf);
    }

    template <typename T>
    auto ParseConfig(NConfig::IConfig& iconfig, TStackUnknownFieldCb uf) {
        return ParseConfigImpl<T>(iconfig, uf);
    }
}
