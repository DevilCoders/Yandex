#include <library/cpp/libgit2_wrapper/signature.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NLibgit2;
using namespace std::chrono_literals;

Y_UNIT_TEST_SUITE(TSignatureTests) {
    Y_UNIT_TEST(CreateItself) {
        TSignature caesar("Julius Caesar", "veni-vedi-vici@rome.it");
        UNIT_ASSERT_VALUES_EQUAL(caesar.Name(), "Julius Caesar");
        UNIT_ASSERT_VALUES_EQUAL(caesar.Email(), "veni-vedi-vici@rome.it");
    }

    Y_UNIT_TEST(ParseFromBuffer) {
        auto shakespeare = TSignature::FromString("William Shakespeare <to-be-or-not-to-be@question.com>");
        UNIT_ASSERT_VALUES_EQUAL(shakespeare.Name(), "William Shakespeare");
        UNIT_ASSERT_VALUES_EQUAL(shakespeare.Email(), "to-be-or-not-to-be@question.com");
        UNIT_ASSERT_UNEQUAL(shakespeare.Time(), TimePoint(0s));
    }

    Y_UNIT_TEST(SetAndGetTime) {
        TSignature shakespeare(
            "William Shakespeare",
            "to-be-or-not-to-be@question.com"
        );
        UNIT_ASSERT_UNEQUAL(shakespeare.Time(), TimePoint(0s));

        TimePoint someTime{123456123456ms};
        shakespeare.SetTime(someTime);
        // fractional part of commit time will be lost during libgit2 adoption
        UNIT_ASSERT_EQUAL(shakespeare.Time(), std::chrono::time_point_cast<std::chrono::seconds>(someTime));
    }
}
