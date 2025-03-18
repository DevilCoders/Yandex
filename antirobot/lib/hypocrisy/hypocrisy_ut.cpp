#include "hypocrisy.h"

#include <library/cpp/resource/resource.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>

#include <array>

using namespace NHypocrisy;


Y_UNIT_TEST_SUITE(Hypocrisy) {
    Y_UNIT_TEST(ParseValidFingerprint) {
        const auto fingerprintStr = NResource::Find("valid_fingerprint.json");
        const auto key = Base64StrictDecode("bY8PcLuZQsG+ZAwW2+pJjjkIP7YJ+vwumnpHLtnOrVw=");

        const auto fingerprintData = NHypocrisy::TFingerprintData::Decrypt(key, fingerprintStr);
        UNIT_ASSERT(fingerprintData.Defined());
        UNIT_ASSERT_VALUES_EQUAL(fingerprintData->Fingerprint["a2"], "Apple Computer, Inc.");
    }
}
