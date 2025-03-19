#include "cast.h"

#include <library/cpp/yson/node/node_io.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/yson/json/json_writer.h>

template <>
NJson::TJsonValue NYT::FromNode<NJson::TJsonValue>(const NYT::TNode& node) {
    TString json;
    TStringOutput str(json);
    TString yson = NYT::NodeToYsonString(node);
    NYT::TJsonWriter writer(&str);
    writer.OnRaw(yson, ::NYson::EYsonType::Node);
    writer.Flush();

    NJson::TJsonValue result;
    NJson::ReadJsonFastTree(json, &result, true);
    return result;
}

template <>
NYT::TNode NYT::ToNode<NJson::TJsonValue>(const NJson::TJsonValue& object) {
    return NYT::NodeFromJsonValue(object);
}
