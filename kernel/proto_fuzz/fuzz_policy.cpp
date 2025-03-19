#include "fuzz_policy.h"

namespace NProtoFuzz {
    void TDefaultFuzzPolicy::DoFuzzInt64(
        const NProtoBuf::FieldDescriptor& /*fd*/,
        i64& value)
    {
        Y_ASSERT(!!Rng);
        const i64 fuzzyValue = TValues::GenInt<i64>(*Rng);
        value = (fuzzyValue == value ? ~fuzzyValue : fuzzyValue);
    }
    void TDefaultFuzzPolicy::DoFuzzUInt64(
        const NProtoBuf::FieldDescriptor& /*fd*/,
        ui64& value)
    {
        Y_ASSERT(!!Rng);
        const ui64 fuzzyValue = TValues::GenInt<ui64>(*Rng);
        value = (fuzzyValue == value ? ~fuzzyValue : fuzzyValue);
    }
    void TDefaultFuzzPolicy::DoFuzzInt32(
        const NProtoBuf::FieldDescriptor& /*fd*/,
        i32& value)
    {
        Y_ASSERT(!!Rng);
        const i32 fuzzyValue = TValues::GenInt<i32>(*Rng);
        value = (fuzzyValue == value ? ~fuzzyValue : fuzzyValue);
    }
    void TDefaultFuzzPolicy::DoFuzzUInt32(
        const NProtoBuf::FieldDescriptor& /*fd*/,
        ui32& value)
    {
        Y_ASSERT(!!Rng);
        const ui32 fuzzyValue = TValues::GenInt<ui32>(*Rng);
        value = (fuzzyValue == value ? ~fuzzyValue : fuzzyValue);
    }
    void TDefaultFuzzPolicy::DoFuzzDouble(
        const NProtoBuf::FieldDescriptor& /*fd*/,
        double& value)
    {
        Y_ASSERT(!!Rng);
        const double fuzzyValue = TValues::GenDouble(*Rng);
        if (fuzzyValue - 1e-6 <= value && fuzzyValue + 1e-6 >= value) {
            value = fuzzyValue + (fuzzyValue >= 0.0f ? -1 : 1);
        } else {
            value = fuzzyValue;
        }
    }
    void TDefaultFuzzPolicy::DoFuzzBool(
        const NProtoBuf::FieldDescriptor& /*fd*/,
        bool& value)
    {
        Y_ASSERT(!!Rng);
        value = !value;
    }
    void TDefaultFuzzPolicy::DoFuzzEnum(
        const NProtoBuf::FieldDescriptor& fd,
        const NProtoBuf::EnumValueDescriptor*& value)
    {
        const NProtoBuf::EnumDescriptor* ed = fd.enum_type();
        if (Y_LIKELY(!!ed)) {
            if (Y_UNLIKELY(ed->value_count() <= 1)) {
                return;
            }
            Y_ASSERT(!!Rng);
            const size_t fuzzyOffset = Rng->Uniform(1, ed->value_count());
            const size_t valueIndex = value->index();
            value = ed->value((valueIndex + fuzzyOffset) % ed->value_count());
        } else {
            Y_ASSERT(false);
        }
    }
    void TDefaultFuzzPolicy::DoFuzzString(
        const NProtoBuf::FieldDescriptor& /*fd*/,
        TString& value)
    {
        Y_ASSERT(!!Rng);
        const TString fuzzyValue = TValues::GenString(*Rng, SmallStringLen, MaxStringLen);
        value = (fuzzyValue.size() == value.size() ? fuzzyValue + "X" : fuzzyValue);
    }
 } // NProtoFuzz
