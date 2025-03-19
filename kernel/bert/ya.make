LIBRARY()

OWNER(
    g:neural-search
    e-shalnov
)

PEERDIR(
    dict/libs/segmenter
    dict/mt/libs/nn
    dict/mt/libs/nn/ynmt
    dict/mt/libs/nn/ynmt/config_helper
    dict/mt/libs/nn/ynmt/extra
    library/cpp/float16
    library/cpp/time_provider
)

IF (USE_GPU)
    PEERDIR(dict/mt/libs/nn/ynmt_backend/gpu)
    CXXFLAGS(-DUSE_GPU)
ELSE()
    PEERDIR(dict/mt/libs/nn/ynmt_backend/cpu)
ENDIF()

SRCS(
    bert_interface.cpp
    batch_processor.cpp
    tokenizer.cpp
)

END()

RECURSE(
    examples
)

