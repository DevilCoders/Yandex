template <typename Archive>
void serialize(
    Archive& ar,
    const unsigned int /* version */)
{{{#FIELD}}
    ar & boost::serialization::make_nvp("{{FIELD_NAME}}", {{FIELD_NAME}}_);{{/FIELD}}
}
