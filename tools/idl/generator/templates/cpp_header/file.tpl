#pragma once{{#IMPORT_SECTION}}
{{#IMPORT}}
#include {{IMPORT_PATH}}{{/IMPORT}}{{/IMPORT_SECTION}}
{{#NAMESPACE}}
namespace {{NAME}} {{{/NAMESPACE}}{{#FWD_DECLS}}
{{#FWD_DECL}}
class {{TYPE_NAME}};{{/FWD_DECL}}{{/FWD_DECLS}}{{#CHILD}}

{{>CHILD}}{{/CHILD}}
{{>BITFIELD_OPERATORS}}{{>WEAK_REF_CREATE_PLATFORMS}}
{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}{{#CLOSING_NAMESPACE_separator}}
{{/CLOSING_NAMESPACE_separator}}{{/CLOSING_NAMESPACE}}{{#SERIALIZATION}}

namespace boost {
namespace serialization {{{#SERIAL}}

{{>SERIAL}}{{/SERIAL}}

} // namespace serialization
} // namespace boost{{/SERIALIZATION}}{{#TRAITS}}
{{#RUNTIME_NAMESPACE}}{{#NAMESPACE}}
namespace {{NAME}} {{{/NAMESPACE}}{{/RUNTIME_NAMESPACE}}
namespace bindings {{{#TYPE}}

template <>
struct BindingTraits<{{TYPE_NAME}}> : public BaseBindingTraits {{{#IS_BIT_FIELD}}
    static constexpr bool isBitField = true;
    static constexpr const char* const javaWrapperSimpleName = "Integer";{{/IS_BIT_FIELD}}{{#IS_STRUCT}}
    static constexpr bool isStruct = true;{{#IS_BRIDGED}}
    static constexpr bool isBridged = true;{{/IS_BRIDGED}}{{/IS_STRUCT}}{{#IS_VARIANT}}
    static constexpr bool isVariant = true;{{/IS_VARIANT}}{{#IS_INTERFACE}}
    using TopMostBaseType = {{TOP_MOST_BASE_TYPE}};{{#IS_STRONG}}
    static constexpr bool isStrongInterface = true;{{/IS_STRONG}}{{#IS_SHARED}}
    static constexpr bool isSharedInterface = true;{{/IS_SHARED}}{{#IS_WEAK}}
    static constexpr bool isWeakInterface = true;{{/IS_WEAK}}
    static constexpr const char* const javaBindingUndecoratedName = "{{JAVA_BINDING_UNDECORATED_NAME}}";{{/IS_INTERFACE}}{{#IS_LISTENER}}
    static constexpr bool isListener = true;{{#IS_STRONG}}
    static constexpr bool isStrongListener = true;{{/IS_STRONG}}{{#IS_WEAK}}
    static constexpr bool isWeakListener = true;{{/IS_WEAK}}{{/IS_LISTENER}}
    static constexpr const char* const javaUndecoratedName = "{{JAVA_UNDECORATED_NAME}}";
    static constexpr const char* const objectiveCName = "{{OBJECTIVE_C_NAME}}";
    static constexpr const char* const cppName = "{{NATIVE_NAME}}";
};{{/TYPE}}

} // namespace bindings
{{#RUNTIME_NAMESPACE}}{{#CLOSING_NAMESPACE}}} // namespace {{NAME}}{{#CLOSING_NAMESPACE_separator}}
{{/CLOSING_NAMESPACE_separator}}{{/CLOSING_NAMESPACE}}{{/RUNTIME_NAMESPACE}}{{/TRAITS}}
