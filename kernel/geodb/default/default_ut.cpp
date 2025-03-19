#include "default.h"

#include <kernel/country_data/countries.h>
#include <kernel/geodb/countries.h>
#include <kernel/geodb/geodb.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(DefaultGeoDBTests) {
    Y_UNIT_TEST(TestDefaultGeoDB) {
        const auto& geodb = NGeoDB::DefaultGeoDB();
        UNIT_ASSERT_VALUES_EQUAL(geodb.Find(NGeoDB::RUSSIA_ID)->GetId(), NGeoDB::RUSSIA_ID);
        UNIT_ASSERT_VALUES_EQUAL(geodb.Find(NGeoDB::UKRAINE_ID)->GetId(), NGeoDB::UKRAINE_ID);
        UNIT_ASSERT_VALUES_EQUAL(geodb.Find(NGeoDB::BELARUS_ID)->GetId(), NGeoDB::BELARUS_ID);
        UNIT_ASSERT_VALUES_EQUAL(geodb.Find(NGeoDB::KAZAKHSTAN_ID)->GetId(), NGeoDB::KAZAKHSTAN_ID);
        UNIT_ASSERT_VALUES_EQUAL(geodb.Find(NGeoDB::TURKEY_ID)->GetId(), NGeoDB::TURKEY_ID);

        UNIT_ASSERT_VALUES_EQUAL(geodb.Find(NGeoDB::RUSSIA_ID)->GetCountryId(), NGeoDB::RUSSIA_ID);
        UNIT_ASSERT_VALUES_EQUAL(geodb.Find(NGeoDB::UKRAINE_ID)->GetCountryId(), NGeoDB::UKRAINE_ID);
        UNIT_ASSERT_VALUES_EQUAL(geodb.Find(NGeoDB::BELARUS_ID)->GetCountryId(), NGeoDB::BELARUS_ID);
        UNIT_ASSERT_VALUES_EQUAL(geodb.Find(NGeoDB::KAZAKHSTAN_ID)->GetCountryId(), NGeoDB::KAZAKHSTAN_ID);
        UNIT_ASSERT_VALUES_EQUAL(geodb.Find(NGeoDB::TURKEY_ID)->GetCountryId(), NGeoDB::TURKEY_ID);
    }

    Y_UNIT_TEST(TestPresenceOfCountriesFromKernelCountryData) {
        const auto& geodb = NGeoDB::DefaultGeoDB();
        for (const auto country : GetRelevCountries()) {
            UNIT_ASSERT_VALUES_EQUAL(geodb.Find(country)->GetId(), country);
            UNIT_ASSERT_VALUES_EQUAL(geodb.Find(country)->GetCountryId(), country);
        }
    }
}
