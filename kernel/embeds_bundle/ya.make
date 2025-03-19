OWNER(ilnurkh)

LIBRARY()

SRCS(
    embeds_bundle.cpp
)

PEERDIR(
    kernel/dssm_applier/begemot
    kernel/dssm_applier/decompression
    kernel/lingboost
    kernel/embeds_bundle/protos
    library/cpp/string_utils/base64
)

END()
