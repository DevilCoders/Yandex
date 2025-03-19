PROGRAM(highlighter)

OWNER(
    chbdr
    g:mt
)

SRCS(
    main.cpp

    bert_highlighter.cpp
    util.cpp
)

PEERDIR(
    dict/mt/libs/nn/ynmt
    dict/mt/libs/nn/ynmt_backend
    dict/mt/libs/nn/ynmt_backend/cpu
    dict/mt/libs/nn/ynmt_backend/gpu_if_supported
    kernel/bert
    library/cpp/getopt
    library/cpp/json
    util
)

IF (USE_GPU)
    CFLAGS(-DUSE_GPU)
ENDIF()

END()

