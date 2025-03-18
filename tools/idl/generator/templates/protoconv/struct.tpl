{{#HAS_BODY}}{{CPP_TYPE_FULL_NAME}} {{IDL_OBJECT_NAME}};{{#FIELD}}

{{#SIMPLE}}{{#OPTIONAL}}if (proto{{PB_TYPE_NAME}}.has_{{PB_FIELD_NAME}}()) {
    {{/OPTIONAL}}{{IDL_OBJECT_NAME}}.{{IDL_FIELD_NAME}} = {{#BRIDGED_FIELD}}std::make_shared<{{IDL_FIELD_TYPE}}::element_type>({{/BRIDGED_FIELD}}{{>FIELD_DECODE}}{{#BRIDGED_FIELD}}){{/BRIDGED_FIELD}};{{#OPTIONAL}}
}{{/OPTIONAL}}{{/SIMPLE}}{{#VECTOR}}if (proto{{PB_TYPE_NAME}}.{{PB_FIELD_NAME}}_size() > 0) {
    {{IDL_OBJECT_NAME}}.{{IDL_FIELD_NAME}}->reserve(proto{{PB_TYPE_NAME}}.{{PB_FIELD_NAME}}_size());
    for (const auto& item : proto{{PB_TYPE_NAME}}.{{PB_FIELD_NAME}}()) {
        {{IDL_OBJECT_NAME}}.{{IDL_FIELD_NAME}}->push_back({{>FIELD_DECODE}});
    }
}{{/VECTOR}}{{/FIELD}}

return {{IDL_OBJECT_NAME}};{{/HAS_BODY}}{{#HAS_NO_BODY}}return { };{{/HAS_NO_BODY}}
