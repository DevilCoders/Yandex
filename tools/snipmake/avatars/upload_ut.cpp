#include "upload.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/neh/neh.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <library/cpp/string_utils/base64/base64.h>

Y_UNIT_TEST_SUITE(TestMdsAvatarsUploadMessage) {
    Y_UNIT_TEST(Test) {
        const TString blob = Base64Decode("R0lGODlhDwAPAKECAAAAzMzM/////wAAACwAAAAADwAPAAACIISPeQHsrZ5ModrLlN48CXF8m2iQ3YmmKqVlRtW4MLwWACH+H09wdGltaXplZCBieSBVbGVhZCBTbWFydFNhdmVyIQAAOw==");
        NNeh::TMessage msg;
        const bool ok = NMdsAvatars::CreateUploadMessage(msg, true, "snippets_images", "megatest", blob);
        UNIT_ASSERT_VALUES_EQUAL(ok, true);
        const TString res = TString{TStringBuf("POST /put-snippets_images/megatest HTTP/1.1\r\nHost: avatars-int.mdst.yandex.net:13000\r\nAccept: application/json\r\nContent-Type: multipart/form-data; boundary=dfgfsdfdhwyewcncnxw\r\nContent-Length: 241\r\n\r\n--dfgfsdfdhwyewcncnxw\r\nContent-Disposition: form-data; name=\"file\"; filename=\"file\"\r\nContent-Length: 106\r\n\r\nGIF89a\x0F\0\x0F\0\xA1\2\0\0\0\xCC\xCC\xCC\xFF\xFF\xFF\xFF\0\0\0,\0\0\0\0\x0F\0\x0F\0\0\2 \x84\x8Fy\1\xEC\xAD\x9EL\xA1\xDA\xCB\x94\xDE<\tq|\x9Bh\x90\xDD\x89\xA6*\245eF\xD5\2700\xBC\x16\0!\xFE\x1FOptimized by Ulead SmartSaver!\0\0;\r\n--dfgfsdfdhwyewcncnxw--\r\n"sv)};
        UNIT_ASSERT_VALUES_EQUAL(msg.Data, res);
    }
}

