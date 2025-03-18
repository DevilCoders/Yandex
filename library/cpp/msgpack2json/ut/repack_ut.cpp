#include <iostream>
#include <fstream>

#include <util/stream/zlib.h>
#include <util/stream/file.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>

#include "msgpack2json.h"

using namespace std;

Y_UNIT_TEST_SUITE(MSGPACK2JSON_REPACK_TEST) {
    Y_UNIT_TEST(RealDataTest) {
        TString expected = "{\"bindings\":[],\"caller_query\":\"\",\"event_id\":null,\"experiment_id\":null,\"hit_log_id\":null,\"http_status\":null,\"lang\":\"ru\",\"line_number\":5816714651,\"norm_url\":\"http://www.autolada.ru/viewtopic.php?start=2950&t=371026\",\"orig_url\":\"http://autolada.ru/viewtopic.php?t=371026&start=2950\",\"page_id\":69122,\"producer_id\":\"f0f5f3c4-3b14-4fb1-84ff-9162822deeb1\",\"request_time\":null,\"resp_body\":null,\"resp_length\":null,\"show_time\":1478075738,\"text\":\"dNCU0J3QoF_Qm9Cd0KAgKNGH0LDRgdGC0YwgMzcpIC0g0J_RgNC-0YHQvNC-0YLRgCDRgtC10LzRiyA6OiBBVVRPTEFEQS5SVQo=\",\"timestamp\":null,\"uniq_id\":1066832391474342727,\"upstream_response_time\":null,\"url\":\"http://www.autolada.ru/viewtopic.php?start=2950&t=371026\",\"url_md5\":13550782916001002848,\"version\":null}";
        const char* fname = "/library/cpp/msgpack2json/ut/id1066832391474342727.compressed";
        TUnbufferedFileInput f(ArcadiaSourceRoot() + fname);
        const TString decompressed(TZLibDecompress(&f).ReadAll());

        // deserialize it.
        msgpack::unpacked msg;
        msgpack::unpack(msg, decompressed.data(), decompressed.length());

        // convert msgpack to JSON.
        msgpack::object obj = msg.get();
        NJson::TJsonValue v;
        NMsgpack2Json::Msgpack2Json(obj, &v);
        TStringStream out;
        NJson::WriteJson(&out, &v, false, true);
        UNIT_ASSERT_VALUES_EQUAL(out.Str(), expected);
    }
}
