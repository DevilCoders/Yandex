Y_BENCHMARK()

OWNER(g:morphology)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/testing/benchmark
    library/cpp/streams/factory
    library/cpp/langs
    kernel/lemmer/new_engine
    kernel/lemmer/new_dict/rus/builtin
    kernel/lemmer/new_dict/eng/builtin
    kernel/lemmer/new_dict/kaz/builtin
)

END()
