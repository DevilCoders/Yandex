#include <kernel/random_log_factors/factor_types.h>
#include <kernel/random_log_factors/proto/random_log_meta_factors.pb.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/algorithm.h>
#include <util/generic/serialized_enum.h>


Y_UNIT_TEST_SUITE(TRandomLogMetaFactors) {
    Y_UNIT_TEST(RandomLogQueryFactorProtoEqualEnum) {
        using NRandomLogFactors::ERandomLogQueryFactor;

        TSet<TString> enumNames;
        const auto& enumValues = GetEnumAllValues<ERandomLogQueryFactor>();
        Transform(
            enumValues.begin(),
            enumValues.end(),
            std::inserter(enumNames, enumNames.end()),
            [](const ERandomLogQueryFactor& value) {return ToString(value);}
        );

        TSet<TString> protoNames;
        constexpr TStringBuf factorPrefix = "RandomLog";
        const auto& descriptor = NRandomLogMetaFactors::TRandomLogMetaFactors::descriptor();
        for (int i = 0; i < descriptor->field_count(); ++i) {
            const TString& fieldName = descriptor->field(i)->name();
            if (fieldName.StartsWith(factorPrefix)) {
                protoNames.insert(fieldName);
            }
        }

        UNIT_ASSERT_VALUES_EQUAL(enumNames, protoNames);
    }
}
