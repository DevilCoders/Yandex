#include <library/cpp/config/domscheme.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TestConfigTraits) {
    Y_UNIT_TEST(ValueFromBool) {
        auto res = TConfigTraits::Value(true);
        UNIT_ASSERT(res.IsA<bool>());
        UNIT_ASSERT_VALUES_EQUAL(res.Get<bool>(), true);
    }

    Y_UNIT_TEST(ValueFromStringLiteral) {
        auto res = TConfigTraits::Value("foobar");
        UNIT_ASSERT(res.IsA<TConfigTraits::TStringType>());
        UNIT_ASSERT_STRINGS_EQUAL(res.Get<TConfigTraits::TStringType>(), "foobar");
    }

    Y_UNIT_TEST(IsValidPrimitive) {
        {
            auto someBool = NConfig::TConfig(NConfig::ConstructValue(true));
            UNIT_ASSERT(TConfigTraits::IsValidPrimitive(bool{}, &someBool));
            UNIT_ASSERT(TConfigTraits::IsValidPrimitive(TString{}, &someBool));
            UNIT_ASSERT(!TConfigTraits::IsArray(&someBool));
            UNIT_ASSERT(!TConfigTraits::IsDict(&someBool));
        }

        {
            auto someString1 = NConfig::TConfig(NConfig::ConstructValue("foobar"));
            UNIT_ASSERT(!TConfigTraits::IsValidPrimitive(bool{}, &someString1));
            UNIT_ASSERT(TConfigTraits::IsValidPrimitive(TString{}, &someString1));
            UNIT_ASSERT(!TConfigTraits::IsArray(&someString1));
            UNIT_ASSERT(!TConfigTraits::IsDict(&someString1));
        }

        {
            auto someString2 = NConfig::TConfig(NConfig::ConstructValue("true"));
            UNIT_ASSERT(TConfigTraits::IsValidPrimitive(bool{}, &someString2));
            UNIT_ASSERT(TConfigTraits::IsValidPrimitive(TString{}, &someString2));
            UNIT_ASSERT(!TConfigTraits::IsArray(&someString2));
            UNIT_ASSERT(!TConfigTraits::IsDict(&someString2));
        }

        {
            auto someArray = NConfig::TConfig::ReadJson("[]");
            UNIT_ASSERT(!TConfigTraits::IsValidPrimitive(bool{}, &someArray));
            UNIT_ASSERT(!TConfigTraits::IsValidPrimitive(TString{}, &someArray));
            UNIT_ASSERT(TConfigTraits::IsArray(&someArray));
            UNIT_ASSERT(!TConfigTraits::IsDict(&someArray));
        }

        {
            auto someDict = NConfig::TConfig::ReadJson("{}");
            UNIT_ASSERT(!TConfigTraits::IsValidPrimitive(bool{}, &someDict));
            UNIT_ASSERT(!TConfigTraits::IsValidPrimitive(TString{}, &someDict));
            UNIT_ASSERT(!TConfigTraits::IsArray(&someDict));
            UNIT_ASSERT(TConfigTraits::IsDict(&someDict));
        }
    }
}
