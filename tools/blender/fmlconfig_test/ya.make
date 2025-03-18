UNITTEST()

OWNER(epar)

PEERDIR(
    library/cpp/json
    library/cpp/resource
    library/cpp/scheme
    library/cpp/testing/unittest
)

SRCS(
    fmlconfig_ut.cpp
)

RESOURCE(
    ${ARCADIA_ROOT}/search/web/rearrs_upper/rearrange.dynamic/blender/fml_config.json fml_config.json
)

REQUIREMENTS(ram:11)

END()
