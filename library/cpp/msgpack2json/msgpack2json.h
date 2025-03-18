#pragma once

#include <contrib/libs/msgpack/include/msgpack.hpp>
#include <library/cpp/json/json_writer.h>

namespace NMsgpack2Json {
    NJson::TJsonValue Msgpack2Json(const msgpack::object& o);
    void Msgpack2Json(const msgpack::object& o, NJson::TJsonValue* v);
}
