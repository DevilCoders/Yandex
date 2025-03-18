#include "builder.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NTskvFormat {
    Y_UNIT_TEST_SUITE(TLogBuilderTest) {
        Y_UNIT_TEST(ShouldAddSeveralFields) {
            TLogBuilder builder = TLogBuilder("test-log", 123)
                                      .Add("f1", "v1")
                                      .Add("f2", "v2");

            UNIT_ASSERT_STRINGS_EQUAL(builder.Str(), "tskv\ttskv_format=test-log\tunixtime=123\tf1=v1\tf2=v2");
        }

        Y_UNIT_TEST(ShouldEscapeFieldName) {
            TLogBuilder builder = TLogBuilder("test-log", 123)
                                      .Add("f=1", "v1");

            UNIT_ASSERT_STRINGS_EQUAL(builder.Str(), "tskv\ttskv_format=test-log\tunixtime=123\tf\\=1=v1");
        }

        Y_UNIT_TEST(ShouldEscapeFieldValue) {
            TLogBuilder builder = TLogBuilder("test-log", 123)
                                      .Add("f1", TStringBuf("\t\n\r\0\\=\"", 7));

            UNIT_ASSERT_STRINGS_EQUAL(builder.Str(), "tskv\ttskv_format=test-log\tunixtime=123\tf1=\\t\\n\\r\\0\\\\\\=\\\"");
        }

        Y_UNIT_TEST(ShouldAddNewLine) {
            TLogBuilder builder = TLogBuilder("test-log", 123)
                                      .End();

            UNIT_ASSERT_STRINGS_EQUAL(builder.Str(), "tskv\ttskv_format=test-log\tunixtime=123\n");
        }

        Y_UNIT_TEST(DefaultBuilderAcceptsSingleValue) {
            TLogBuilder builder = TLogBuilder()
                                      .Add("f1", "v1");
            UNIT_ASSERT_STRINGS_EQUAL(builder.Str(), "f1=v1");
        }

        Y_UNIT_TEST(DefaultBuilderAcceptsMultipleValues) {
            TLogBuilder builder = TLogBuilder()
                                      .Add("f1", "v1")
                                      .Add("f2", "v2");
            UNIT_ASSERT_STRINGS_EQUAL(builder.Str(), "f1=v1\tf2=v2");
        }

        Y_UNIT_TEST(DefaultBuilderBeginsAndEnds) {
            TLogBuilder builder = TLogBuilder()
                                      .Begin("test-log", 123)
                                      .End();
            UNIT_ASSERT_STRINGS_EQUAL(builder.Str(), "tskv\ttskv_format=test-log\tunixtime=123\n");
        }

        Y_UNIT_TEST(RepeatedBeginAddsLine) {
            TLogBuilder builder;
            builder.Begin("test-log", 123).End();
            builder.Begin("test-log", 246);
            UNIT_ASSERT_STRINGS_EQUAL(builder.Str(), "tskv\ttskv_format=test-log\tunixtime=123\ntskv\ttskv_format=test-log\tunixtime=246");
        }
    }

}
