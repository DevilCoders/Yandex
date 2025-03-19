#include "factory.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/file.h>
#include <util/system/tempfile.h>


static const TString Data = "abcabcabcabc";

class TCompressorFactoryTest : public TTestBase {
    UNIT_TEST_SUITE(TCompressorFactoryTest);
        UNIT_TEST(TestNoCompression);
        UNIT_TEST(TestZLibCompression);
        UNIT_TEST(TestLzCompression);
        UNIT_TEST(TestGzipCompression);
        UNIT_TEST(TestBlockCodecCompression);
    UNIT_TEST_SUITE_END();

public:
    void TestNoCompression();
    void TestZLibCompression();
    void TestLzCompression();
    void TestGzipCompression();
    void TestBlockCodecCompression();
private:
    void Compress(TCompressorFactory::EFormat format);
    void Decompress();
    void DecompressNoAlloc();

    TString Compressed;

};

void TCompressorFactoryTest::Compress(TCompressorFactory::EFormat format) {
    Compressed.clear();
    TStringOutput strOut(Compressed);
    TCompressorFactory::Compress(&strOut, Data, format);
}

void TCompressorFactoryTest::Decompress() {
    TStringInput strIn(Compressed);
    UNIT_ASSERT_EQUAL(TCompressorFactory::Decompress(&strIn), Data);
}

void TCompressorFactoryTest::DecompressNoAlloc() {
    TString out;
    TStringInput strIn(Compressed);
    TCompressorFactory::Decompress(&strIn, out);
    UNIT_ASSERT_EQUAL(out, Data);
}

void TCompressorFactoryTest::TestNoCompression() {
    Compress(TCompressorFactory::NO_COMPRESSION);
    Decompress();
    DecompressNoAlloc();
}
void TCompressorFactoryTest::TestZLibCompression() {
    Compress(TCompressorFactory::ZLIB_DEFAULT);
    Decompress();
    DecompressNoAlloc();
}
void TCompressorFactoryTest::TestLzCompression() {
    Compress(TCompressorFactory::LZ_LZ4);
    Decompress();
    DecompressNoAlloc();
}
void TCompressorFactoryTest::TestGzipCompression() {
    Compress(TCompressorFactory::GZIP_DEFAULT);
    Decompress();
    DecompressNoAlloc();
}
void TCompressorFactoryTest::TestBlockCodecCompression() {
    Compress(TCompressorFactory::BC_ZSTD_08_1);
}

UNIT_TEST_SUITE_REGISTRATION(TCompressorFactoryTest);
