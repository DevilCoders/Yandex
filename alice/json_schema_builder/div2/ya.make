######################################################################
# WARNING: this code was automatically generated by alice/json_schema_builder/tool,
# don't edit it manually.
######################################################################

LIBRARY()

OWNER(
    g:alice
)

PEERDIR(
    alice/json_schema_builder/runtime
)

SRCS(
    log_id_filler.cpp
    validate.cpp
)

GENERATE_ENUM_SERIALIZATION(
    enums.h
)

END()
