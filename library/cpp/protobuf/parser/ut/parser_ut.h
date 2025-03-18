#pragma once

#include "parser.h"

#include <library/cpp/protobuf/parser/ut/parser_ut.pb.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/noncopyable.h>

class TTestSource: public TNonCopyable {
public:
    TTestSource(int f1, int f2)
        : Field1(f1)
        , Field2(f2)
    {
        Exception[0] = Exception[1] = Exception[2] = false;
    }

    int Field1;
    int Field2;

    bool Exception[3];

    void Throw(int i) const {
        if (Exception[i])
            ythrow yexception();
    }
};

namespace NProtoParser {
    template <>
    class TDefaultMerger<TTestSource>: public TBaseMerger {
    public:
        class TEnableDefaultMerger;
        void operator()(TMessageParser& parser, const TTestSource& src) const {
            src.Throw(0);
            parser.Merge(TStringBuf("optional_int32"), src.Field1);

            src.Throw(1);
            parser.Merge(TStringBuf("repeated_int64"), src.Field2);

            src.Throw(2);
        }

        void operator()(TMessageParser& parser, const FieldDescriptor* desc, const TTestSource& src) const {
            src.Throw(0);
            parser.Merge(desc, src.Field1);
        }
    };

}

struct TTestParser {
    TTestParser()
        : Msg(Proto)
    {
    }

    NParserUt::TTestMessage Proto;
    NProtoParser::TMessageParser Msg;
};

#define TEST_CONVERSION_SINGLE_FIELD_WITH_LABEL(field, label)    \
    {                                                            \
        TTestParser parser;                                      \
        parser.Msg.Merge(#label "_" #field, from);               \
        UNIT_ASSERT(parser.Proto.has_##label##_##field());       \
        COMPARE_RESULTS(result, parser.Proto.label##_##field()); \
    }

#define TEST_CONVERSION_REPEATED_FIELD(field)                         \
    {                                                                 \
        TTestParser parser;                                           \
        parser.Msg.Merge("repeated_" #field, from);                   \
        UNIT_ASSERT_EQUAL(1, parser.Proto.repeated_##field##_size()); \
        COMPARE_RESULTS(result, parser.Proto.repeated_##field(0));    \
                                                                      \
        parser.Msg.Merge("repeated_" #field, from);                   \
        UNIT_ASSERT_EQUAL(2, parser.Proto.repeated_##field##_size()); \
        COMPARE_RESULTS(result, parser.Proto.repeated_##field(0));    \
        COMPARE_RESULTS(result, parser.Proto.repeated_##field(1));    \
    }

#define TEST_CONVERSION_OPTIONAL_FIELD(field) TEST_CONVERSION_SINGLE_FIELD_WITH_LABEL(field, optional)
#define TEST_CONVERSION_REQUIRED_FIELD(field) TEST_CONVERSION_SINGLE_FIELD_WITH_LABEL(field, required)

#define TEST_CONVERSION_SINGLE_FIELD(field) \
    TEST_CONVERSION_OPTIONAL_FIELD(field)   \
    TEST_CONVERSION_REQUIRED_FIELD(field)

#define TEST_CONVERSION_FIELD(field, from_, result_) \
    {                                                \
        const auto from = from_;                     \
        const auto result = result_;                 \
        TEST_CONVERSION_SINGLE_FIELD(field)          \
        TEST_CONVERSION_REPEATED_FIELD(field)        \
    }

#define TEST_BAD_CAST_SINGLE_FIELD_WITH_LABEL(field, label)                                  \
    {                                                                                        \
        TTestParser parser;                                                                  \
        parser.Proto.set_##label##_##field(old);                                             \
        UNIT_ASSERT_EXCEPTION(parser.Msg.Merge(#label "_" #field, from), TBadCastException); \
        COMPARE_RESULTS(old, parser.Proto.label##_##field());                                \
    }                                                                                        \
    {                                                                                        \
        TTestParser parser;                                                                  \
        UNIT_ASSERT_EXCEPTION(parser.Msg.Merge(#label "_" #field, from), TBadCastException); \
        UNIT_ASSERT(!parser.Proto.has_##label##_##field());                                  \
    }

#define TEST_BAD_CAST_REPEATED_FIELD(field)                                                   \
    {                                                                                         \
        TTestParser parser;                                                                   \
        parser.Proto.add_repeated_##field(old);                                               \
        UNIT_ASSERT_EXCEPTION(parser.Msg.Merge("repeated_" #field, from), TBadCastException); \
        UNIT_ASSERT_EQUAL(1, parser.Proto.repeated_##field##_size());                         \
        COMPARE_RESULTS(old, parser.Proto.repeated_##field(0));                               \
    }                                                                                         \
    {                                                                                         \
        TTestParser parser;                                                                   \
        UNIT_ASSERT_EXCEPTION(parser.Msg.Merge("repeated_" #field, from), TBadCastException); \
        UNIT_ASSERT(!parser.Proto.repeated_##field##_size());                                 \
    }

#define TEST_BAD_CAST_OPTIONAL_FIELD(field) TEST_BAD_CAST_SINGLE_FIELD_WITH_LABEL(field, optional)
#define TEST_BAD_CAST_REQUIRED_FIELD(field) TEST_BAD_CAST_SINGLE_FIELD_WITH_LABEL(field, required)

#define TEST_BAD_CAST_SINGLE_FIELD(field) \
    TEST_BAD_CAST_OPTIONAL_FIELD(field)   \
    TEST_BAD_CAST_REQUIRED_FIELD(field)

#define TEST_BAD_CAST_FIELD(field, old_, from_) \
    {                                           \
        const auto old = old_;                  \
        const auto from = from_;                \
        TEST_BAD_CAST_SINGLE_FIELD(field)       \
        TEST_BAD_CAST_REPEATED_FIELD(field)     \
    }
