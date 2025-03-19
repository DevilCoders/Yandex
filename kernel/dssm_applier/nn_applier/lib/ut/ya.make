UNITTEST()

OWNER(
    agusakov
    g:neural-search
)

SIZE(MEDIUM)

DATA(
    sbr://175016747 # dssm_sample.bin
    sbr://883840920 # assessor.appl
    sbr://1660207068 # titles_lw.bin
    sbr://1660206135 # titles_lb.bin
    sbr://1660206509 # titles_ll3.bin
    sbr://1660381625 # left_titles_2020_08_07.txt
)

SRCS(
    dependencies_ut.cpp
    split_model_ut.cpp
    test_blob_hash_set.cpp
    test_layers.cpp
    test_metadata.cpp
    test_predot_scale_layer.cpp
    test_updatable_dict.cpp
    test_version.cpp
    trigram_index_ut.cpp
)

FORK_TESTS()

PEERDIR(
    kernel/dssm_applier/nn_applier/lib
    library/cpp/testing/unittest
)

END()
