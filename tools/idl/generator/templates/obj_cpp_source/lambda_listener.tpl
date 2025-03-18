{{#METHOD}}{{LAMBDA_METHOD_SCOPE}}{{FUNCTION_NAME}} {{FUNCTION_NAME:x-uncap}}(
    {{OBJC_TYPE_NAME}} handler)
{
    return [handler]({{#CPP_PARAM}}{{#PARAM}}
        {{#CONST_PARAM}}const {{/CONST_PARAM}}{{PARAM_TYPE}}{{#REF_PARAM}}&{{/REF_PARAM}} {{PARAM_NAME}}{{#PARAM_separator}},{{/PARAM_separator}}{{/PARAM}}{{#CPP_PARAM_separator}},{{/CPP_PARAM_separator}}{{/CPP_PARAM}})
    {
        if (!handler) {
            return;
        }

        {{RUNTIME_NAMESPACE_PREFIX}}verify{{#UI_THREAD}}Ui{{/UI_THREAD}}{{#BG_THREAD}}BgPlatform{{/BG_THREAD}}{{#ANY_THREAD}}AnyThread{{/ANY_THREAD}}AndRun([&] {
            handler({{#NIL_BEFORE}}
                {}{{#NIL_BEFORE_separator}},{{/NIL_BEFORE_separator}}{{/NIL_BEFORE}}{{#COMMA_BEFORE}},{{/COMMA_BEFORE}}{{#PARAM}}{{#PARAM_ENUM}}
                static_cast<{{OBJC_PARAM_TYPE}}>({{PARAM_NAME}}){{/PARAM_ENUM}}{{#TO_PLATFORM}}
                {{RUNTIME_NAMESPACE_PREFIX}}bindings::ios::toPlatform({{PARAM_NAME}}){{/TO_PLATFORM}}{{#PARAM_NSERROR}}
                {{RUNTIME_NAMESPACE_PREFIX}}makeError({{PARAM_NAME}}){{/PARAM_NSERROR}}{{#ALLOC_INIT_INTERFACE}}
                [[{{OBJC_INTERFACE_TYPE}} alloc] initWithNative:{{#MOVE}}std::move({{/MOVE}}{{PARAM_NAME}}{{#MOVE}}){{/MOVE}}]{{/ALLOC_INIT_INTERFACE}}{{#PARAM_separator}},{{/PARAM_separator}}{{/PARAM}}{{#COMMA_AFTER}},{{/COMMA_AFTER}}{{#NIL_AFTER}}
                {}{{#NIL_AFTER_separator}},{{/NIL_AFTER_separator}}{{/NIL_AFTER}});
        });
    };
}{{#METHOD_separator}}
{{/METHOD_separator}}{{/METHOD}}
