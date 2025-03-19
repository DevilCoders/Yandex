#include <kernel/lingboost/freq.h>
#include <kernel/text_machine/interface/stream_constants.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/serialized_enum.h>

using namespace NLingBoost;

Y_UNIT_TEST_SUITE(ConstTest) {
    Y_UNIT_TEST(TestRegionId) {
        Cdbg << TRegionId(0) << " "
            << TRegionId(225) << " "
            << TRegionId(213) << Endl;

        UNIT_ASSERT_EQUAL(ToString(TRegionId(0)), "World");
        UNIT_ASSERT_EQUAL(ToString(TRegionId(225)), "Country(ru)");
        UNIT_ASSERT_EQUAL(ToString(TRegionId(213)), "SmallRegion(213)");
        UNIT_ASSERT_EQUAL(ToString(TRegionId(-1)), "NotRegion(-1)");

        UNIT_ASSERT_EQUAL(FromString<TRegionId>("0"), TRegionId(0));
        UNIT_ASSERT_EQUAL(FromString<TRegionId>("225"), TRegionId(225));
        UNIT_ASSERT_EQUAL(FromString<TRegionId>("213"), TRegionId(213));

        UNIT_ASSERT_EQUAL(FromString<TRegionId>("World"), TRegionId(0));
        UNIT_ASSERT_EQUAL(FromString<TRegionId>("Country(225)"), TRegionId(225));
        UNIT_ASSERT_EQUAL(FromString<TRegionId>("SmallRegion(213)"), TRegionId(213));

        UNIT_ASSERT_EXCEPTION(FromString<TRegionId>("Wrold"), yexception);
        UNIT_ASSERT_EXCEPTION(FromString<TRegionId>("World(0)"), yexception);
        UNIT_ASSERT_EXCEPTION(FromString<TRegionId>("Country()"), yexception);
        UNIT_ASSERT_EXCEPTION(FromString<TRegionId>("Country(x)"), yexception);
        UNIT_ASSERT_EXCEPTION(FromString<TRegionId>("Country(225"), yexception);
        UNIT_ASSERT_EXCEPTION(FromString<TRegionId>("Country(225))"), yexception);
        UNIT_ASSERT_EXCEPTION(FromString<TRegionId>("Country(213)"), yexception);

        UNIT_ASSERT_EQUAL(FromString<TRegionId>("Country(ru)"), TRegionId(225));
    }

    Y_UNIT_TEST(TestRegionIdOrdering) {
        UNIT_ASSERT(TRegionId::Russia() < TRegionId::ClassCountry());
        UNIT_ASSERT(TRegionId::Russia() < TRegionId::ClassWorld());
        UNIT_ASSERT(TRegionId::ClassCountry() < TRegionId::ClassWorld());
        UNIT_ASSERT(TRegionId::World() < TRegionId::ClassCountry()); // all regions go before classes
        UNIT_ASSERT(TRegionId::World() < TRegionId::ClassWorld());
    }

    Y_UNIT_TEST(TestTypeDescr) {
        UNIT_ASSERT(TStreamTypeDescr::Instance().HasValueIndex(TStream::OneClick));
        UNIT_ASSERT(!TStreamTypeDescr::Instance().HasValueIndex(100500));
        UNIT_ASSERT_VALUES_EQUAL(TStreamTypeDescr::Instance().GetValueLiteral(TStream::OneClick), "OneClick");
        UNIT_ASSERT_VALUES_EQUAL(TStreamTypeDescr::Instance().GetTypeCppName(), "::NLingBoost::TStream");
        UNIT_ASSERT_VALUES_EQUAL(TStreamTypeDescr::Instance().GetValueScopePrefix(), "::NLingBoost::TStream::");
    }

    Y_UNIT_TEST(NoValuesForgottenInStaticArrays) {
        UNIT_ASSERT_VALUES_EQUAL(TExpansionStruct::GetValues().size() + 1, GetEnumAllValues<TExpansionStruct::EType>().size());
        UNIT_ASSERT_VALUES_EQUAL(TMatchStruct::GetValues().size() + 1, GetEnumAllValues<TMatchStruct::EType>().size());
        UNIT_ASSERT_VALUES_EQUAL(TBaseIndexLayer::GetValues().size() + 1, GetEnumAllValues<TBaseIndexLayer::EType>().size());
        UNIT_ASSERT_VALUES_EQUAL(TBaseIndexStruct::GetValues().size() + 1, GetEnumAllValues<TBaseIndexStruct::EType>().size());
        UNIT_ASSERT_VALUES_EQUAL(TStreamStruct::GetValues().size() + 1, GetEnumAllValues<TStreamStruct::EType>().size());
        UNIT_ASSERT_VALUES_EQUAL(TRegionClassStruct::GetValues().size() + 1, GetEnumAllValues<TRegionClassStruct::EType>().size());
        UNIT_ASSERT_VALUES_EQUAL(TRequestForm::GetValues().size() + 1, GetEnumAllValues<TRequestForm::EType>().size());
        UNIT_ASSERT_VALUES_EQUAL(TMatchPrecisionStruct::GetValues().size() + 1, GetEnumAllValues<TMatchPrecisionStruct::EType>().size());
        UNIT_ASSERT_VALUES_EQUAL(TConstraintStruct::GetValues().size() + 1, GetEnumAllValues<TConstraintStruct::EType>().size());
        UNIT_ASSERT_VALUES_EQUAL(TWordFreqStruct::GetValues().size() + 1, GetEnumAllValues<TWordFreqStruct::EType>().size());
    }
}
