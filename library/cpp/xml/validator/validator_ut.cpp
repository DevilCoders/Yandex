#include "validator.h"

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NXml;

Y_UNIT_TEST_SUITE() {
    Y_UNIT_TEST(Validation) {
        TValidator validator(ArcadiaSourceRoot() + "/library/cpp/xml/validator/test_data/country.xsd");

        TDocument valid_doc(ArcadiaSourceRoot() + "/library/cpp/xml/validator/test_data/valid.xml");
        UNIT_ASSERT_VALUES_EQUAL(validator.IsValid(valid_doc), true);

        TDocument invalid_doc(ArcadiaSourceRoot() + "/library/cpp/xml/validator/test_data/invalid.xml");
        UNIT_ASSERT_VALUES_EQUAL(validator.IsValid(invalid_doc), false);
    }
}
