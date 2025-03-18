#include <iostream>
#include <fstream>

#include <library/cpp/testing/unittest/registar.h>

#include "msgpack2json.h"

using namespace std;

Y_UNIT_TEST_SUITE(MSGPACK2JSON_SIMPLE_TEST) {
    Y_UNIT_TEST(TestArray) {
        TString expected = "[\"message 1\",\"message 2\",\"message 3\"]";

        msgpack::sbuffer buffer;

        // serializes multiple objects into one message containing an array using msgpack::packer.
        msgpack::packer<msgpack::sbuffer> pk(&buffer);
        pk.pack_array(3);
        pk.pack(string("message 1"));
        pk.pack(string("message 2"));
        pk.pack(string("message 3"));

        // deserialize it.
        msgpack::unpacked msg;
        msgpack::unpack(msg, buffer.data(), buffer.size());

        // convert msgpack to JSON.
        msgpack::object obj = msg.get();
        NJson::TJsonValue v;
        NMsgpack2Json::Msgpack2Json(obj, &v);
        TStringStream out;
        NJson::WriteJson(&out, &v, false, true);
        UNIT_ASSERT_VALUES_EQUAL(out.Str(), expected);
    }

    Y_UNIT_TEST(TestArrayEmpty) {
        TString expected = "[]";

        msgpack::sbuffer buffer;

        // serializes multiple objects into one message containing an array using msgpack::packer.
        msgpack::packer<msgpack::sbuffer> pk(&buffer);
        pk.pack_array(0);

        // deserialize it.
        msgpack::unpacked msg;
        msgpack::unpack(msg, buffer.data(), buffer.size());

        // convert msgpack to JSON.
        msgpack::object obj = msg.get();
        NJson::TJsonValue v;
        NMsgpack2Json::Msgpack2Json(obj, &v);
        TStringStream out;
        NJson::WriteJson(&out, &v, false, true);
        UNIT_ASSERT_VALUES_EQUAL(out.Str(), expected);
    }

    Y_UNIT_TEST(TestMap) {
        TString expected = "{\"x\":3,\"y\":3.4321}";

        msgpack::sbuffer buffer;

        // serializes multiple objects into one message containing a map using msgpack::packer.
        msgpack::packer<msgpack::sbuffer> pk(&buffer);
        pk.pack_map(2);
        pk.pack(string("x"));
        pk.pack(3);
        pk.pack(string("y"));
        pk.pack(3.4321);

        // deserialize it.
        msgpack::unpacked msg;
        msgpack::unpack(msg, buffer.data(), buffer.size());

        // convert msgpack to JSON.
        msgpack::object obj = msg.get();
        NJson::TJsonValue v;
        NMsgpack2Json::Msgpack2Json(obj, &v);
        TStringStream out;
        NJson::WriteJson(&out, &v, false, true);
        UNIT_ASSERT_VALUES_EQUAL(out.Str(), expected);
    }

    Y_UNIT_TEST(TestEmptyMap) {
        TString expected = "{}";

        msgpack::sbuffer buffer;

        // serializes multiple objects into one message containing a map using msgpack::packer.
        msgpack::packer<msgpack::sbuffer> pk(&buffer);
        pk.pack_map(0);

        // deserialize it.
        msgpack::unpacked msg;
        msgpack::unpack(msg, buffer.data(), buffer.size());

        // convert msgpack to JSON.
        msgpack::object obj = msg.get();
        NJson::TJsonValue v;
        NMsgpack2Json::Msgpack2Json(obj, &v);
        TStringStream out;
        NJson::WriteJson(&out, &v, false, true);
        UNIT_ASSERT_VALUES_EQUAL(out.Str(), expected);
    }

    Y_UNIT_TEST(TestString) {
        TString expected = "\"string\""; // "[\"string\"]";

        msgpack::sbuffer buffer;

        // serializes multiple objects into one message containing a map using msgpack::packer.
        msgpack::packer<msgpack::sbuffer> pk(&buffer);
        pk.pack_str(strlen("string"));
        pk.pack_str_body("string", strlen("string"));

        // deserialize it.
        msgpack::unpacked msg;
        msgpack::unpack(msg, buffer.data(), buffer.size());

        // convert msgpack to JSON.
        msgpack::object obj = msg.get();
        NJson::TJsonValue v;
        NMsgpack2Json::Msgpack2Json(obj, &v);
        TStringStream out;
        NJson::WriteJson(&out, &v, false, true);
        cout << out.Str() << endl;
        cout << "QQQ" << endl;
        cout << expected << endl;
        UNIT_ASSERT_VALUES_EQUAL(out.Str(), expected);
    }

    Y_UNIT_TEST(TestMixed) {
        TString expected = "[{},[],{\"key\":123},[234,[],456]]";

        msgpack::sbuffer buffer;

        // serializes multiple objects into one message containing a map using msgpack::packer.
        msgpack::packer<msgpack::sbuffer> pk(&buffer);
        pk.pack_array(4);
        pk.pack_map(0);
        pk.pack_array(0);
        pk.pack_map(1);
        pk.pack(string("key"));
        pk.pack(123);
        pk.pack_array(3);
        pk.pack(234);
        pk.pack_array(0);
        pk.pack(456);

        // deserialize it.
        msgpack::unpacked msg;
        msgpack::unpack(msg, buffer.data(), buffer.size());

        // convert msgpack to JSON.
        msgpack::object obj = msg.get();
        NJson::TJsonValue v;
        NMsgpack2Json::Msgpack2Json(obj, &v);
        TStringStream out;
        NJson::WriteJson(&out, &v, false, true);
        UNIT_ASSERT_VALUES_EQUAL(out.Str(), expected);
    }

    Y_UNIT_TEST(TestMixed2) {
        TString expected = "{\"key1\":{},\"key2\":[],\"key3\":{\"subkey\":123},\"key4\":[234,{},456]}";

        msgpack::sbuffer buffer;

        // serializes multiple objects into one message containing a map using msgpack::packer.
        msgpack::packer<msgpack::sbuffer> pk(&buffer);
        pk.pack_map(4);
        pk.pack(string("key1"));
        pk.pack_map(0);
        pk.pack(string("key2"));
        pk.pack_array(0);
        pk.pack(string("key3"));
        pk.pack_map(1);
        pk.pack(string("subkey"));
        pk.pack(123);
        pk.pack(string("key4"));
        pk.pack_array(3);
        pk.pack(234);
        pk.pack_map(0);
        pk.pack(456);

        // deserialize it.
        msgpack::unpacked msg;
        msgpack::unpack(msg, buffer.data(), buffer.size());

        // convert msgpack to JSON.
        msgpack::object obj = msg.get();
        NJson::TJsonValue v;
        NMsgpack2Json::Msgpack2Json(obj, &v);
        TStringStream out;
        NJson::WriteJson(&out, &v, false, true);
        UNIT_ASSERT_VALUES_EQUAL(out.Str(), expected);
    }

    /* FAILED: (msgpack::v1::insufficient_bytes) insufficient bytes
    Y_UNIT_TEST(TestEmpty) {
        TString expected = "";

        msgpack::sbuffer buffer;

        // serializes multiple objects into one message containing a map using msgpack::packer.
        msgpack::packer<msgpack::sbuffer> pk(&buffer);

        // deserialize it.
        msgpack::unpacked msg;
        msgpack::unpack(msg, buffer.data(), buffer.size());

        // convert msgpack to JSON.
        msgpack::object obj = msg.get();
        NJson::TJsonValue v;
        NMsgpack2Json::Msgpack2Json(obj, &v);
        TStringStream out;
        NJson::WriteJson(&out, &v, false, true);
        UNIT_ASSERT_VALUES_EQUAL(out.Str(), expected);
    }*/
}
