LIBRARY()

OWNER(
    g:neural-search
    alexbykov
)

PEERDIR(
    kernel/dssm_applier/optimized_model
    kernel/searchlog
)

SRCS(
    GLOBAL test_utils.cpp
    optimized_model_params.proto
)

END()
