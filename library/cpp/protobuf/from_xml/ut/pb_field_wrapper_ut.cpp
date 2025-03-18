#include <library/cpp/protobuf/from_xml/pb.h>
#include <library/cpp/protobuf/from_xml/pb_field_wrapper.h>
#include <library/cpp/protobuf/from_xml/ut/proto/basic.pb.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/yexception.h>

#include <cstdint>

using namespace NProtobufFromXml;

namespace {
    struct Fixture {
        TestMessage Message;
        TPbFieldWrapper RequiredField{Message, "required_field"};
        TPbFieldWrapper OptionalField{Message, "optional_field"};
        TPbFieldWrapper RepeatedField{Message, "repeated_field"};
    };

}

Y_UNIT_TEST_SUITE(TPbFieldWrapperTests) {
    Y_UNIT_TEST(Ctor_givenUnknownField_throws) {
        TestMessage msg;
        UNIT_ASSERT_EXCEPTION(
            TPbFieldWrapper(msg, "unknown_field"),
            yexception);
    }

    Y_UNIT_TEST(RequireType_givenNonMatchingType_throws) {
        Fixture f;
        UNIT_ASSERT_EXCEPTION(
            f.OptionalField.RequireType(PbFieldDescriptor::TYPE_STRING),
            yexception);
    }

    Y_UNIT_TEST(Type_forAnyField_returnsExpectedValue) {
        Fixture f;
        UNIT_ASSERT_EQUAL(
            f.OptionalField.Type(),
            PbFieldDescriptor::TYPE_BOOL);
        UNIT_ASSERT_EQUAL(
            f.RequiredField.Type(),
            PbFieldDescriptor::TYPE_UINT32);
        UNIT_ASSERT_EQUAL(
            f.RepeatedField.Type(),
            PbFieldDescriptor::TYPE_UINT64);
    }

    Y_UNIT_TEST(Name_forAnyField_returnsExpectedValue) {
        TestMessage msg;
        const auto fieldName = "optional_field";
        UNIT_ASSERT_EQUAL(
            TPbFieldWrapper(msg, fieldName).Name(),
            fieldName);
    }

    Y_UNIT_TEST(ParentName_forAnyField_returnsExpectedValue) {
        TestMessage msg;
        UNIT_ASSERT_EQUAL(
            TPbFieldWrapper(msg, "optional_field").ParentName(),
            "TestMessage");
    }

    Y_UNIT_TEST(IsRepeated_forRepeatedField_returnsTrue) {
        UNIT_ASSERT(Fixture{}.RepeatedField.IsRepeated());
    }

    Y_UNIT_TEST(IsRepeated_forNonRepeatedField_returnsFalse) {
        UNIT_ASSERT(!Fixture{}.OptionalField.IsRepeated());
    }

    Y_UNIT_TEST(IsOptional_forOptionalField_returnsTrue) {
        UNIT_ASSERT(Fixture{}.OptionalField.IsOptional());
    }

    Y_UNIT_TEST(IsOptional_forNonOptionalField_returnsFalse) {
        UNIT_ASSERT(!Fixture{}.RepeatedField.IsOptional());
    }

    Y_UNIT_TEST(Add_forRepeatedField_successfullyAddsValue) {
        Fixture f;
        const ui64 fieldValue = 42;
        f.RepeatedField.Add(fieldValue);
        UNIT_ASSERT_EQUAL(f.Message.repeated_field_size(), 1);
        UNIT_ASSERT_EQUAL(f.Message.repeated_field(0), fieldValue);
    }

    Y_UNIT_TEST(Set_forRequiredField_successfullySetsValue) {
        Fixture f;
        const ui32 fieldValue = 42;
        f.RequiredField.Set(fieldValue);
        UNIT_ASSERT_EQUAL(f.Message.required_field(), fieldValue);
    }
}
