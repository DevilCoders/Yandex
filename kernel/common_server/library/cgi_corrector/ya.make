LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/reqid
    library/cpp/cgiparam
    library/cpp/http/misc
)

SRCS(
    abstract.cpp
    fake.cpp
)

END()
