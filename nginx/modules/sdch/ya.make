LIBRARY()

OWNER(
    wawa
    toshik
    g:contrib
)

BUILD_ONLY_IF(LINUX)

NO_UTIL()

CFLAGS(-Wno-unused-parameter)

PEERDIR(
    contrib/nginx/core/src/http
    contrib/tools/open-vcdiff
)

ADDINCL(
    contrib/nginx/core/objs
)

SRCS(
    sdch_autoauto_handler.cc
    sdch_config.cc
    sdch_dictionary.cc
    sdch_dictionary_factory.cc
    sdch_dump_handler.cc
    sdch_encoding_handler.cc
    sdch_fastdict_factory.cc
    sdch_handler.cc
    sdch_main_config.cc
    sdch_module.cc
    sdch_output_handler.cc
    sdch_request_context.cc
)

END()
