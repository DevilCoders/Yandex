#include "msgpack2json.h"

namespace NMsgpack2Json {
    using namespace NJson;

    TJsonValue Msgpack2Json(const msgpack::object& o) {
        TJsonValue value;
        Msgpack2Json(o, &value);
        return value;
    }

    void Msgpack2Json(const msgpack::object& o, TJsonValue* value) {
        switch (o.type) {
            case msgpack::type::ARRAY:
                value->SetType(JSON_ARRAY);
                for (size_t i = 0; i < o.via.array.size; ++i) {
                    const auto* p = o.via.array.ptr + i;
                    value->AppendValue(Msgpack2Json(*p));
                }
                break;

            case msgpack::type::MAP:
                value->SetType(JSON_MAP);
                for (size_t i = 0; i < o.via.map.size; ++i) {
                    const auto* p = o.via.map.ptr + i;
                    Y_ENSURE(p->key.type == msgpack::type::STR, "key should be str");
                    TStringBuf key(p->key.via.str.ptr, p->key.via.str.size);
                    (*value)[key] = Msgpack2Json(p->val);
                }
                break;

            case msgpack::type::NIL:
                *value = JSON_NULL;
                break;

            case msgpack::type::BOOLEAN:
                *value = o.via.boolean ? true : false;
                break;

            case msgpack::type::POSITIVE_INTEGER:
                *value = o.via.u64;
                break;

            case msgpack::type::NEGATIVE_INTEGER:
                *value = o.via.i64;
                break;

            case msgpack::type::FLOAT:
                *value = o.via.f64;
                break;

            case msgpack::type::STR:
                *value = TStringBuf(o.via.str.ptr, o.via.str.size);
                break;

            case msgpack::type::BIN:
                *value = TStringBuf(o.via.bin.ptr, o.via.bin.size);
                break;

            case msgpack::type::EXT:
                ythrow yexception() << "Msgpack2Json: currently unsupported, TBD";
                // TODO
                //break;

            default:
                ythrow yexception() << "Msgpack2Json: invalid type";
        }
    }
}
