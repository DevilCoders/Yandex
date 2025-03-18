#include <library/cpp/html/html5/parse.h>
#include <library/cpp/html/storage/storage.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TBrokenEncodings) {
    Y_UNIT_TEST(TRussianTags) {
        const TStringBuf data =
            "<noinde\321\205></noinde\321\205>"sv;
        NHtml::TStorage storage;
        NHtml::TParserResult processor(storage);
        NHtml5::ParseHtml(data, &processor);
    }
}
