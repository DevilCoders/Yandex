LIBRARY()

OWNER(
    sankear
    g:base
)

SRCS(
    kosher_portion_writer.h
    kosher_portion_sampler.h
    kosher_portion_reader.h
    kosher_portion_io.h
    kosher_portion_io_factory.h
)

PEERDIR(
    library/cpp/offroad/codec
    library/cpp/offroad/tuple
    library/cpp/offroad/key
    library/cpp/offroad/custom
    kernel/doom/standard_models
    kernel/doom/standard_models_storage
)

END()

