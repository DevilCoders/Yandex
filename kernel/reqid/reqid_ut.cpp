#include <library/cpp/testing/unittest/registar.h>

#include "reqid.h"

Y_UNIT_TEST_SUITE(TReqIdTest) {
    Y_UNIT_TEST(TestReqIdParse) {
        TInstant timestamp;
        TString reqIdClass;

        // Generic valid reqid
        ReqIdParse("1446196133223974-475465283771163670307066-man1-0856", timestamp, reqIdClass);
        UNIT_ASSERT_EQUAL(timestamp, TInstant::MicroSeconds(1446196133223974));
        UNIT_ASSERT_VALUES_EQUAL(reqIdClass, TString());

        // generic valid YP reqid
        ReqIdParse("1581586508852135-809412815943362792500173-production-app-host-vla-web-yp-5.vla.yp-c.yandex.net", timestamp, reqIdClass);
        UNIT_ASSERT_EQUAL(timestamp, TInstant::MicroSeconds(1581586508852135));
        UNIT_ASSERT_VALUES_EQUAL(reqIdClass, TString());

        // with reqid class
        ReqIdParse("1372693715306836-2175494974-margarita-RXML", timestamp, reqIdClass);
        UNIT_ASSERT_EQUAL(timestamp, TInstant::MicroSeconds(1372693715306836));
        UNIT_ASSERT_VALUES_EQUAL(reqIdClass, "RXML");

        // reqid class with reask
        ReqIdParse("1372693715306836-2175494974-margarita-RXML-REASK", timestamp, reqIdClass);
        UNIT_ASSERT_EQUAL(timestamp, TInstant::MicroSeconds(1372693715306836));
        UNIT_ASSERT_VALUES_EQUAL(reqIdClass, "RXML");

        // A bit more exotique reqid class with reask
        ReqIdParse("1372693715306836-2175494974-margarita-REASK-RXML", timestamp, reqIdClass);
        UNIT_ASSERT_EQUAL(timestamp, TInstant::MicroSeconds(1372693715306836));
        UNIT_ASSERT_VALUES_EQUAL(reqIdClass, "RXML");

        // YP reqid with reask
        ReqIdParse("1372693715306836-2175494974-production-app-host-vla-web-yp-5.vla.yp-c.yandex.net-REASK-RXML", timestamp, reqIdClass);
        UNIT_ASSERT_EQUAL(timestamp, TInstant::MicroSeconds(1372693715306836));
        UNIT_ASSERT_VALUES_EQUAL(reqIdClass, "RXML");

        // reqid class with page
        ReqIdParse("1372693715306836-2175494974-margarita-RXML-p100", timestamp, reqIdClass);
        UNIT_ASSERT_EQUAL(timestamp, TInstant::MicroSeconds(1372693715306836));
        UNIT_ASSERT_VALUES_EQUAL(reqIdClass, "RXML");

        // reask without reqid class
        ReqIdParse("1372693715306836-2175494974-margarita-REASK", timestamp, reqIdClass);
        UNIT_ASSERT_EQUAL(timestamp, TInstant::MicroSeconds(1372693715306836));
        UNIT_ASSERT_VALUES_EQUAL(reqIdClass, TString());

        // Strange/weird and bad cases

        // Empty reqid
        ReqIdParse("", timestamp, reqIdClass);
        UNIT_ASSERT_EQUAL(timestamp, TInstant::MicroSeconds(0));
        UNIT_ASSERT_VALUES_EQUAL(reqIdClass, TString());

        // Invalid reqid
        ReqIdParse("-", timestamp, reqIdClass);
        UNIT_ASSERT_EQUAL(timestamp, TInstant::MicroSeconds(0));
        UNIT_ASSERT_VALUES_EQUAL(reqIdClass, TString());

        // Weird reqid
        ReqIdParse("abcde100", timestamp, reqIdClass);
        UNIT_ASSERT_EQUAL(timestamp, TInstant::MicroSeconds(0));
        UNIT_ASSERT_VALUES_EQUAL(reqIdClass, TString());

        // Duplicated separators (=> empty chunks) 1
        ReqIdParse("1372693715306836--2175494974-margarita-RXML-p100", timestamp, reqIdClass);
        UNIT_ASSERT_EQUAL(timestamp, TInstant::MicroSeconds(1372693715306836));
        UNIT_ASSERT_VALUES_EQUAL(reqIdClass, "RXML");

        // Duplicated reqid class
        ReqIdParse("1372693715306836-2175494974-margarita-RXML-SXML", timestamp, reqIdClass);
        UNIT_ASSERT_EQUAL(timestamp, TInstant::MicroSeconds(1372693715306836));
        UNIT_ASSERT_VALUES_EQUAL(reqIdClass, "RXML");

        // Duplicated separators (=> empty chunks) 2
        ReqIdParse("1372693715306836-2175494974--margarita-RXML-p100", timestamp, reqIdClass);
        UNIT_ASSERT_EQUAL(timestamp, TInstant::MicroSeconds(1372693715306836));
        UNIT_ASSERT_VALUES_EQUAL(reqIdClass, "RXML");

        // Invalid timestamp (overflow)
        ReqIdParse("14461961332239743838383838383838323423-475465283771163670307066-man1-0856", timestamp, reqIdClass);
        UNIT_ASSERT_EQUAL(timestamp, TInstant::MicroSeconds(0));
        UNIT_ASSERT_VALUES_EQUAL(reqIdClass, TString());
    }

    Y_UNIT_TEST(TestValidateReqId) {
        bool isValid;

        // Generic valid reqid
        isValid = ValidateReqId("1446196133223974-475465283771163670307066-man1-0856");
        UNIT_ASSERT_VALUES_EQUAL(true, isValid);

        // with reqid class
        isValid = ValidateReqId("1372693715306836-1016066059592407229789376-vla9-1337-RXML");
        UNIT_ASSERT_VALUES_EQUAL(true, isValid);

        // yet another valid reqid
        isValid = ValidateReqId("1372693715306836-892301783551566666842204-sas4-1234-RXML-p100-REASK");
        UNIT_ASSERT_VALUES_EQUAL(true, isValid);

        // Complex bud still valid hostname with suffix
        isValid = ValidateReqId("1516731429544714-510775158890269966336137-abacaba_0-1.sas.yp-c.yandex.net-p100-XML");
        UNIT_ASSERT_VALUES_EQUAL(true, isValid);

        // Strange/weird and bad cases

        // Empty reqid
        isValid = ValidateReqId("");
        UNIT_ASSERT_VALUES_EQUAL(false, isValid);

        // Invalid reqid
        isValid = ValidateReqId("-");
        UNIT_ASSERT_VALUES_EQUAL(false, isValid);

        // too short timestamp
        isValid = ValidateReqId("137269371530-892301783551566666842204-sas4-1234-RXML-p100-REASK");
        UNIT_ASSERT_VALUES_EQUAL(false, isValid);

        // Weird reqid
        isValid = ValidateReqId("abcde100");
        UNIT_ASSERT_VALUES_EQUAL(false, isValid);

        // hostname is absent
        isValid = ValidateReqId("1516727628943904-10287853171264717370");
        UNIT_ASSERT_VALUES_EQUAL(false, isValid);

        // reqid without timestamp
        isValid = ValidateReqId("-10287853171264717370-vla2-9999");
        UNIT_ASSERT_VALUES_EQUAL(false, isValid);

        // reqid with whitespace character
        isValid = ValidateReqId("-10287853171264717370-vla2-9999 -p100");
        UNIT_ASSERT_VALUES_EQUAL(false, isValid);

        isValid = ValidateReqId("-10287853171264717370-vla2-9999-TCH\n-p100");
        UNIT_ASSERT_VALUES_EQUAL(false, isValid);

        // Duplicated separators (=> empty chunks) 1
        isValid = ValidateReqId("1372693715306836--1535467176633507644536165-man6-1357-RXML-p100");
        UNIT_ASSERT_VALUES_EQUAL(false, isValid);

        // Duplicated separators (=> empty chunks) 2
        isValid = ValidateReqId("1372693715306836-1194594517558947140892601--vla1-1234-RXML-p100");
        UNIT_ASSERT_VALUES_EQUAL(false, isValid);

        // Notorious reqid from real stream
        isValid = ValidateReqId("vins_1516095098817134");
        UNIT_ASSERT_VALUES_EQUAL(false, isValid);

        // Reqid with forbidden chars in hostname
        isValid = ValidateReqId("1516731429544714-510775158890269966336137-host1,/\\+=*&^%$#@!\"':;?-p100");
        UNIT_ASSERT_VALUES_EQUAL(false, isValid);

        // Short reqid
        isValid = ValidateReqId("1516731429544714-510775158890269966336137-a");
        UNIT_ASSERT_VALUES_EQUAL(false, isValid);

        // myt dc
        isValid = ValidateReqId("1533297689852310-6397453775863943611-myt1-1731-msk-myt-bass-prod-18274-vins");
        UNIT_ASSERT_VALUES_EQUAL(true, isValid);

        // iva dc
        isValid = ValidateReqId("1533297689852310-6397453775863943611-iva1-1234");
        UNIT_ASSERT_VALUES_EQUAL(true, isValid);

        // Substring of a valid reqId
        TStringBuf fullString = "1533297689852310-6397453775863943611-iva1-1234";
        TStringBuf substringToMatch = fullString.Head(1); // only the first symbol
        isValid = ValidateReqId(substringToMatch);
        UNIT_ASSERT_VALUES_EQUAL(false, isValid);

        // yp hosts
        isValid = ValidateReqId("1581700046699980-1019172984632449351100192-production-app-host-man-web-yp-5.man.yp-c.yandex.net");
        UNIT_ASSERT_VALUES_EQUAL(true, isValid);

        isValid = ValidateReqId("1581700038967400-921206895955546429000568-production-report-man-web-yp-5.man.yp-c.yandex.net-XML");
        UNIT_ASSERT_VALUES_EQUAL(true, isValid);
    }
}
