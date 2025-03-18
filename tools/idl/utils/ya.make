OWNER(
    g:geoapps_infra
)

LIBRARY()

ADDINCL(
    GLOBAL tools/idl/utils/include
)


SRCS(
    paths.cpp
    common.cpp
    errors.cpp
    exception.cpp
)

END()
