LIBRARY()

OWNER(
    elric
    g:base
)

SRCS(
    dummy.cpp
    fat_key_reader.h
    fat_key_seeker.h
    fat_key_writer.h
    fwd.h
    key_input_buffer.h
    key_model.h
    key_output_buffer.h
    key_reader.h
    key_sampler.h
    key_table.h
    key_writer.h
    null_key_writer.h
)

PEERDIR(
    library/cpp/offroad/codec
    library/cpp/offroad/fat
    library/cpp/offroad/tuple
    library/cpp/offroad/utility
)

END()
