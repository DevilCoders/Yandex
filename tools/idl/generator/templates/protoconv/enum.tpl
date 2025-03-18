switch (proto{{PB_ENUM_NAME}}) {{{#CASE}}
    case {{PB_FIELD_NAME}}:
        return {{IDL_FIELD_NAME}};{{/CASE}}
}

throw {{RUNTIME_NAMESPACE_PREFIX}}RuntimeError() <<
    "Unrecognized .proto enum constant";