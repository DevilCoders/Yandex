PROGRAM(vw_ops)

OWNER(
    g:adult
    tobo
    vdf
)

PEERDIR(
    library/cpp/streams/factory
    library/cpp/vowpalwabbit
    library/cpp/getopt
    ml/differential_evolution
)

SRCS(
    fml_pool.cpp
    main.cpp
    pack_model.cpp
)

END()
