#include "parser_ut.h"

namespace NImpl {
    void FieldMergeTest1();
    void FieldMergeTest2();
    void FieldMergeTest3();
    void FieldMergeTest4();
    void FieldMergeTest5();
    void FieldMergeTest6();
    void FieldMergeTest7();
}

class TProtoParserTest: public TTestBase {
    UNIT_TEST_SUITE(TProtoParserTest);
    UNIT_TEST(FieldMergeTest1);
    UNIT_TEST(FieldMergeTest2);
    UNIT_TEST(FieldMergeTest3);
    UNIT_TEST(FieldMergeTest4);
    UNIT_TEST(FieldMergeTest5);
    UNIT_TEST(FieldMergeTest6);
    UNIT_TEST(FieldMergeTest7);
    UNIT_TEST(FlagsTest);
    UNIT_TEST(MergeFromOtherTypesTest);
    UNIT_TEST(DelimiterTest);
    UNIT_TEST_SUITE_END();

public:
    void FieldMergeTest1() {
        NImpl::FieldMergeTest1();
    }

    void FieldMergeTest2() {
        NImpl::FieldMergeTest2();
    }

    void FieldMergeTest3() {
        NImpl::FieldMergeTest3();
    }

    void FieldMergeTest4() {
        NImpl::FieldMergeTest4();
    }

    void FieldMergeTest5() {
        NImpl::FieldMergeTest5();
    }

    void FieldMergeTest6() {
        NImpl::FieldMergeTest6();
    }

    void FieldMergeTest7() {
        NImpl::FieldMergeTest7();
    }

    void FlagsTest() {
        {
            NParserUt::TTestMessage proto;
            NProtoParser::TMessageParser msg(proto, NProtoParser::TMessageParser::SILENT);
            msg.Merge("unknown_field", 1);
            msg.Merge("optional_int32", "stroka");
        }

        {
            NParserUt::TTestMessage proto;
            NProtoParser::TMessageParser msg(proto, 0);
            UNIT_ASSERT_EXCEPTION(msg.Merge("unknown_field", 1), yexception);
            UNIT_ASSERT_EXCEPTION(msg.Merge("optional_int32", "stroka"), TBadCastException);
        }

        {
            NParserUt::TTestMessage proto;
            NProtoParser::TMessageParser msg(proto);
            msg.Merge("unknown_field", 1);
            UNIT_ASSERT_EXCEPTION(msg.Merge("optional_int32", "stroka"), TBadCastException);
        }
    }

    void MergeFromOtherTypesTest() {
        {
            TTestSource src(1, 2);
            NParserUt::TTestMessage proto;
            NProtoParser::TMessageParser msg(proto);
            msg.Merge(src);
            UNIT_ASSERT_EQUAL(1, proto.optional_int32());
            UNIT_ASSERT_EQUAL(1, proto.repeated_int64_size());
            UNIT_ASSERT_EQUAL(2, proto.repeated_int64(0));

            src.Field1 = 3;
            src.Field2 = 4;

            msg.Merge(src);
            UNIT_ASSERT_EQUAL(3, proto.optional_int32());
            UNIT_ASSERT_EQUAL(2, proto.repeated_int64_size());
            UNIT_ASSERT_EQUAL(2, proto.repeated_int64(0));
            UNIT_ASSERT_EQUAL(4, proto.repeated_int64(1));
        }

        {
            TTestSource src(1, 2);
            NParserUt::TTestMessage proto;
            NProtoParser::MergeFrom(proto, src);
            UNIT_ASSERT_EQUAL(1, proto.optional_int32());
            UNIT_ASSERT_EQUAL(1, proto.repeated_int64_size());
            UNIT_ASSERT_EQUAL(2, proto.repeated_int64(0));

            src.Field1 = 3;
            src.Field2 = 4;

            NProtoParser::MergeFrom(proto, src);
            UNIT_ASSERT_EQUAL(3, proto.optional_int32());
            UNIT_ASSERT_EQUAL(2, proto.repeated_int64_size());
            UNIT_ASSERT_EQUAL(2, proto.repeated_int64(0));
            UNIT_ASSERT_EQUAL(4, proto.repeated_int64(1));
        }

        {
            TTestSource src(1, 2);
            NParserUt::TTestMessage proto;
            NProtoParser::TMessageParser msg(proto);
            msg.Merge("optional_message", src);
            UNIT_ASSERT(!proto.has_optional_int32());

            UNIT_ASSERT(proto.has_optional_message());
            UNIT_ASSERT_EQUAL(1, proto.optional_message().optional_int32());
            UNIT_ASSERT_EQUAL(1, proto.optional_message().repeated_int64_size());
            UNIT_ASSERT_EQUAL(2, proto.optional_message().repeated_int64(0));

            src.Field1 = 3;
            src.Field2 = 4;

            msg.Merge(TStringBuf("optional_message"), src);
            UNIT_ASSERT(proto.has_optional_message());
            UNIT_ASSERT_EQUAL(3, proto.optional_message().optional_int32());
            UNIT_ASSERT_EQUAL(2, proto.optional_message().repeated_int64_size());
            UNIT_ASSERT_EQUAL(2, proto.optional_message().repeated_int64(0));
            UNIT_ASSERT_EQUAL(4, proto.optional_message().repeated_int64(1));
        }

        {
            TTestSource src(1, 2);
            NParserUt::TTestMessage proto;
            NProtoParser::TMessageParser msg(proto);
            msg.Merge("optional_message", src);
            UNIT_ASSERT(!proto.has_optional_int32());

            UNIT_ASSERT(proto.has_optional_message());
            UNIT_ASSERT_EQUAL(1, proto.optional_message().optional_int32());
            UNIT_ASSERT_EQUAL(1, proto.optional_message().repeated_int64_size());
            UNIT_ASSERT_EQUAL(2, proto.optional_message().repeated_int64(0));

            src.Field1 = 3;
            src.Field2 = 4;
            src.Exception[1] = true;

            UNIT_ASSERT_EXCEPTION(msg.Merge(TString("optional_message"), src), yexception);
            UNIT_ASSERT(proto.has_optional_message());
            UNIT_ASSERT_EQUAL(1, proto.optional_message().optional_int32());
            UNIT_ASSERT_EQUAL(1, proto.optional_message().repeated_int64_size());
            UNIT_ASSERT_EQUAL(2, proto.optional_message().repeated_int64(0));
        }

        {
            TTestSource src(1, 2);
            NParserUt::TTestMessage proto;
            NProtoParser::TMessageParser msg(proto, NProtoParser::TMessageParser::DEFAULT_MODE & ~NProtoParser::TMessageParser::ATOMIC); // disable ATOMIC
            msg.Merge("optional_message", src);
            UNIT_ASSERT(!proto.has_optional_int32());

            UNIT_ASSERT(proto.has_optional_message());
            UNIT_ASSERT_EQUAL(1, proto.optional_message().optional_int32());
            UNIT_ASSERT_EQUAL(1, proto.optional_message().repeated_int64_size());
            UNIT_ASSERT_EQUAL(2, proto.optional_message().repeated_int64(0));

            src.Field1 = 3;
            src.Field2 = 4;
            src.Exception[1] = true;

            UNIT_ASSERT_EXCEPTION(msg.Merge("optional_message", src), yexception);
            UNIT_ASSERT(!proto.has_optional_message());
        }

        {
            TTestSource src(1, 2);
            NParserUt::TTestMessage proto;
            NProtoParser::TMessageParser msg(proto);
            msg.Merge("repeated_message", src);
            UNIT_ASSERT(!proto.has_optional_int32());

            UNIT_ASSERT_EQUAL(1, proto.repeated_message_size());
            UNIT_ASSERT_EQUAL(1, proto.repeated_message(0).optional_int32());
            UNIT_ASSERT_EQUAL(1, proto.repeated_message(0).repeated_int64_size());
            UNIT_ASSERT_EQUAL(2, proto.repeated_message(0).repeated_int64(0));

            src.Field1 = 3;
            src.Field2 = 4;

            msg.Merge("repeated_message", src);
            UNIT_ASSERT_EQUAL(2, proto.repeated_message_size());

            UNIT_ASSERT_EQUAL(1, proto.repeated_message(0).optional_int32());
            UNIT_ASSERT_EQUAL(1, proto.repeated_message(0).repeated_int64_size());
            UNIT_ASSERT_EQUAL(2, proto.repeated_message(0).repeated_int64(0));

            UNIT_ASSERT_EQUAL(3, proto.repeated_message(1).optional_int32());
            UNIT_ASSERT_EQUAL(1, proto.repeated_message(1).repeated_int64_size());
            UNIT_ASSERT_EQUAL(4, proto.repeated_message(1).repeated_int64(0));
        }

        {
            TTestSource src(1, 2);
            NParserUt::TTestMessage proto;
            NProtoParser::TMessageParser msg(proto);
            msg.Merge("repeated_message", src);
            UNIT_ASSERT(!proto.has_optional_int32());

            UNIT_ASSERT_EQUAL(1, proto.repeated_message_size());
            UNIT_ASSERT_EQUAL(1, proto.repeated_message(0).optional_int32());
            UNIT_ASSERT_EQUAL(1, proto.repeated_message(0).repeated_int64_size());
            UNIT_ASSERT_EQUAL(2, proto.repeated_message(0).repeated_int64(0));

            src.Field1 = 3;
            src.Field2 = 4;
            src.Exception[2] = true;

            UNIT_ASSERT_EXCEPTION(msg.Merge("repeated_message", src), yexception);

            UNIT_ASSERT_EQUAL(1, proto.repeated_message_size());
            UNIT_ASSERT_EQUAL(1, proto.repeated_message(0).optional_int32());
            UNIT_ASSERT_EQUAL(1, proto.repeated_message(0).repeated_int64_size());
            UNIT_ASSERT_EQUAL(2, proto.repeated_message(0).repeated_int64(0));
        }
    }

    void DelimiterTest() {
        NParserUt::TTestMessage proto;
        NProtoParser::TMessageParser msg(proto);
        msg.Merge("repeated_double", "1,2,3.3");
        UNIT_ASSERT_EQUAL(3, proto.repeated_double_size());
        UNIT_ASSERT_DOUBLES_EQUAL(1.0, proto.repeated_double(0), 0.001);
        UNIT_ASSERT_DOUBLES_EQUAL(2.0, proto.repeated_double(1), 0.001);
        UNIT_ASSERT_DOUBLES_EQUAL(3.3, proto.repeated_double(2), 0.001);

        UNIT_ASSERT_EXCEPTION(msg.Merge("repeated_float", "1,2,3.3"), TBadCastException); // has no delimiter option

        msg.Merge("repeated_enum", "test1,test2");
        UNIT_ASSERT_EQUAL(2, proto.repeated_enum_size());
        UNIT_ASSERT_EQUAL(NParserUt::test1, proto.repeated_enum(0));
        UNIT_ASSERT_EQUAL(NParserUt::test2, proto.repeated_enum(1));

        msg.Merge("repeated_string", "test1,test2"); // has no delimiter field
        UNIT_ASSERT_EQUAL(1, proto.repeated_string_size());
        UNIT_ASSERT_STRINGS_EQUAL("test1,test2", proto.repeated_string(0));

        msg.Merge("optional_string", "test1,test2"); // has delimiter field but is not repeated
        UNIT_ASSERT_STRINGS_EQUAL("test1,test2", proto.optional_string());
    }
};

UNIT_TEST_SUITE_REGISTRATION(TProtoParserTest);
