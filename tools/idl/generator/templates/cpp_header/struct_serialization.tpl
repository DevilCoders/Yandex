template <typename Archive>
void serialize(
    Archive& ar,
    {{STRUCT_TYPE}}& obj,
    const unsigned int /* version */)
{{{#FIELD}}
    ar & boost::serialization::make_nvp("{{FIELD_NAME}}", {{#BRIDGED}}{{#NOT_OPTIONAL}}*{{/NOT_OPTIONAL}}{{/BRIDGED}}obj.{{FIELD_NAME}});{{/FIELD}}
}
