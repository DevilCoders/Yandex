{{CONSTRUCTOR_NAME}}Binding::{{CONSTRUCTOR_NAME}}Binding(
    id<{{OBJC_TYPE_NAME}}> platformListener)
    : platformListener_(platformListener)
{
}
{{#METHOD}}
{{RESULT_TYPE}} {{CONSTRUCTOR_NAME}}Binding::{{FUNCTION_NAME}}({{#CPP_PARAM}}{{#PARAM}}
    {{#CONST_PARAM}}const {{/CONST_PARAM}}{{PARAM_TYPE}}{{#REF_PARAM}}&{{/REF_PARAM}} {{PARAM_NAME}}{{#PARAM_separator}},{{/PARAM_separator}}{{/PARAM}}{{#CPP_PARAM_separator}},{{/CPP_PARAM_separator}}{{/CPP_PARAM}}){{#CONST_METHOD}} const{{/CONST_METHOD}}
{
    {{#RETURNS_SOMETHING}}return {{/RETURNS_SOMETHING}}{{RUNTIME_NAMESPACE_PREFIX}}verify{{#UI_THREAD}}Ui{{/UI_THREAD}}{{#BG_THREAD}}BgPlatform{{/BG_THREAD}}{{#ANY_THREAD}}AnyThread{{/ANY_THREAD}}AndRun([&] {
        {{#RETURNS_SOMETHING}}return {{#STATIC_CAST_RESULT}}static_cast<{{RESULT_TYPE}}>{{/STATIC_CAST_RESULT}}{{#SIMPLE_RESULT}}{{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toNative<{{RESULT_TYPE}}>{{/SIMPLE_RESULT}}{{#LISTENER_RESULT}}{{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toNative<{{RESULT_TYPE}}>{{/LISTENER_RESULT}}({{/RETURNS_SOMETHING}}[platformListener_ {{OBJC_FUNCTION_NAME}}{{#WITH}}With{{/WITH}}{{#PARAM}}{{OBJC_METHOD_NAME_PARAM_PART}}:{{#PARAM_ENUM}}static_cast<{{OBJC_PARAM_TYPE}}>({{PARAM_NAME}}){{/PARAM_ENUM}}{{#TO_PLATFORM}} {{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toPlatform({{PARAM_NAME}}){{/TO_PLATFORM}}{{#ALLOC_INIT_INTERFACE}} {{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toPlatform({{#MOVE}}std::move({{/MOVE}}{{PARAM_NAME}}{{#MOVE}}){{/MOVE}}){{/ALLOC_INIT_INTERFACE}}{{#PARAM_LISTENER}} {{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toPlatform({{PARAM_NAME}}){{/PARAM_LISTENER}}{{#PARAM_NSERROR}} {{RUNTIME_NAMESPACE_PREFIX}}makeError({{PARAM_NAME}}){{/PARAM_NSERROR}}{{#PARAM_separator}}
            {{/PARAM_separator}}{{/PARAM}}]{{#RETURNS_SOMETHING}}){{/RETURNS_SOMETHING}};
    });
}{{#METHOD_separator}}
{{/METHOD_separator}}{{/METHOD}}
