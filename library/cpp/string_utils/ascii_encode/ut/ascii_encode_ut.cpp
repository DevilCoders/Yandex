#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/string_utils/ascii_encode/ascii_encode.h>
#include <util/string/ascii.h>

Y_UNIT_TEST_SUITE(TEncodeAsciiTest){
    static void TestEncodeDecode(const TString& s){
        TString encoded = "temp";
EncodeAscii(s, &encoded);
const size_t bitCount = s.size() * 8;
const size_t rest = bitCount % 5;
const size_t encodedSize = bitCount / 5 + (rest ? 1 : 0);
UNIT_ASSERT_VALUES_EQUAL(encodedSize, encoded.size());
TString decoded;
DecodeAscii(encoded, &decoded);
UNIT_ASSERT_VALUES_EQUAL(s, decoded);
}

static void TestDecodeEncode(const TString& s) {
    TString decoded = "temp";
    UNIT_ASSERT(DecodeAscii(s, &decoded));
    TString encoded;
    EncodeAscii(decoded, &encoded);
    UNIT_ASSERT_VALUES_EQUAL(s, encoded);
}

Y_UNIT_TEST(EncodeDecodeTest) {
    TestEncodeDecode("");
    TestEncodeDecode("a");
    TestEncodeDecode("\t");
    TestEncodeDecode("bb");
    TestEncodeDecode("ca\t");
}

Y_UNIT_TEST(DecodeEncodeTest) {
    TestDecodeEncode("");
    TestDecodeEncode("SE10000");
}

Y_UNIT_TEST(ComplexTest) {
    TVector<char> result;
    result.push_back('a'); // spoil vector
    TString source = "qqqq12323\t\r\n";

    EncodeAscii(source, &result);

    for (auto c : result) {
        UNIT_ASSERT(c >= 'A' && c <= 'V' || c >= '0' && c <= '9');
    }

    // check that lowercase does not hurt encoding
    TString resultStr(result.data(), result.size());
    resultStr.to_lower();

    TVector<unsigned char> uResult;
    uResult.push_back('b');
    DecodeAscii(resultStr, &uResult);

    UNIT_ASSERT_EQUAL(source.size(), uResult.size());
    for (size_t i = 0; i < source.size(); ++i) {
        UNIT_ASSERT_EQUAL(source[i], char(uResult[i]));
    }
}
}
;
