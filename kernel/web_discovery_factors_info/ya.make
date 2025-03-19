LIBRARY()

LICENSE(YandexNDA)

OWNER(
    g:chats
)

SRCS(
    factor_names.cpp
    GLOBAL ${BINDIR}/factors_gen.cpp
)

PEERDIR(
    kernel/web_factors_info
    mapreduce/yt/interface/protos
)

SPLIT_CODEGEN(kernel/generated_factors_info/factors_codegen factors_gen NWebDiscovery)

END()
