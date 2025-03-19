PROGRAM()

OWNER(
    g:neural-search
    alexbykov
)

PEERDIR(
    kernel/dssm_applier/embeddings_transfer
    kernel/dssm_applier/optimized_model/test_utils
    library/cpp/getopt
    library/cpp/yson/node
)

SRCS(
    main.cpp
    options.cpp
)

GENERATE_ENUM_SERIALIZATION(options.h)

END()
