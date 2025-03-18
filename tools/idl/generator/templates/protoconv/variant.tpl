{{#FIELD}}{{#SIMPLE}}if (proto{{PB_TYPE_NAME}}.has_{{PB_FIELD_NAME}}()) {
    return {{#BRIDGED_FIELD}}std::make_shared<{{IDL_FIELD_TYPE}}::element_type>({{/BRIDGED_FIELD}}{{>FIELD_DECODE}}{{#BRIDGED_FIELD}}){{/BRIDGED_FIELD}};
}{{/SIMPLE}}{{#VECTOR}}if (proto{{PB_TYPE_NAME}}.{{PB_FIELD_NAME}}_size() > 0) {
    auto {{IDL_FIELD_NAME}} = std::make_shared<{{IDL_FIELD_TYPE}}::element_type>();
    for (const auto& item : proto{{PB_TYPE_NAME}}.{{PB_FIELD_NAME}}()) {
        {{IDL_FIELD_NAME}}->push_back({{>FIELD_DECODE}});
    }
    return {{IDL_FIELD_NAME}};
}{{/VECTOR}}{{#FIELD_separator}} else {{/FIELD_separator}}{{/FIELD}}

throw {{RUNTIME_NAMESPACE_PREFIX}}RuntimeError() <<
    "All fields are empty in .proto message when converting it to variant";