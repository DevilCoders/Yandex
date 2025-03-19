LIBRARY()

OWNER(iddqd)

WERROR()

PEERDIR(
    kernel/common_proxy/common
)

SRCS(
    GLOBAL registrar.cpp
    file_writer.cpp
)

END()
