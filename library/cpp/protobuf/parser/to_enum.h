#pragma once

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include <util/generic/cast.h>
#include <util/generic/strbuf.h>
#include <util/string/cast.h>

namespace NProtoParser {
    namespace NEnumConvertor {
#define DISABLE_CONVERT_TO_ENUM(type)                                                 \
    inline const EnumValueDescriptor*                                                 \
    ToEnum(type, const FieldDescriptor*) {                                            \
        ythrow TBadCastException() << "Conversion from " #type " to enum is invalid"; \
    }

        DISABLE_CONVERT_TO_ENUM(float)
        DISABLE_CONVERT_TO_ENUM(double)
        DISABLE_CONVERT_TO_ENUM(bool)

#undef DISABLE_CONVERT_TO_ENUM

        // integers to enum by value
        template <class TInteger>
        std::enable_if_t<std::is_integral<TInteger>::value, const EnumValueDescriptor*>
        ToEnum(TInteger value, const FieldDescriptor* desc) {
            const int val = SafeIntegerCast<int>(value);
            const EnumDescriptor* const enumDesc = desc->enum_type();
            Y_ASSERT(enumDesc);
            const EnumValueDescriptor* const enumValue = enumDesc->FindValueByNumber(val);
            if (!enumValue)
                ythrow TBadCastException() << "Failed to find enumerator value " << val << " in " << enumDesc->name();
            return enumValue;
        }

// string to enum by name or value
#define STRING_TO_ENUM(type)                                                                                        \
    inline const EnumValueDescriptor*                                                                               \
    ToEnum(type value, const FieldDescriptor* desc) {                                                               \
        int byValue = 0;                                                                                            \
        if (TryFromString(value, byValue))                                                                          \
            return ToEnum(byValue, desc);                                                                           \
                                                                                                                    \
        const EnumDescriptor* const enumDesc = desc->enum_type();                                                   \
        Y_ASSERT(enumDesc);                                                                                         \
        const EnumValueDescriptor* const enumValue = enumDesc->FindValueByName(TString(value));                     \
        if (!enumValue)                                                                                             \
            ythrow TBadCastException() << "Failed to find enumerator name " << value << " in " << enumDesc->name(); \
        return enumValue;                                                                                           \
    }

        STRING_TO_ENUM(TStringBuf)
        STRING_TO_ENUM(TString)

#undef STRING_TO_ENUM
    }
}
