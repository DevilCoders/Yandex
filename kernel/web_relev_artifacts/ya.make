LIBRARY()

OWNER(
    ilnurkh
    g:factordev
)

SRCS(
    artifacts_registry.cpp
)

RESOURCE(
    artifacts_registry.proto.txt /kernel/web_relev_artifacts/artifacts_registry.proto.txt
)

PEERDIR(
    kernel/web_relev_artifacts/metadata
    library/cpp/resource
    library/cpp/protobuf/util
)

END()

RECURSE_FOR_TESTS(
    ut
    ut_factors_info
)
