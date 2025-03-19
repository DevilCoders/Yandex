PROTO_LIBRARY()

OWNER(
    g:blender
)

PY_NAMESPACE(blender.saas.proto)

SRCS(
    surplus_components.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
