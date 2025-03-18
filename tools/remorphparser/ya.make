PROGRAM()

OWNER(g:remorph)

ALLOCATOR(LF)

SRCS(
    main.cpp
    arc_reader.cpp
    handler.cpp
    options.cpp
)

PEERDIR(
    FactExtract/Parser/common
    FactExtract/Parser/common/docreaders
    dict/corpus
    kernel/gazetteer
    kernel/gazetteer/richtree
    kernel/indexer/baseproc
    kernel/indexer/direct_text
    kernel/indexer/face
    kernel/indexer/faceproc
    kernel/indexer/parseddoc
    kernel/qtree/richrequest
    kernel/remorph/cascade
    kernel/remorph/common
    kernel/remorph/directtext
    kernel/remorph/engine/char
    kernel/remorph/facts
    kernel/remorph/info
    kernel/remorph/input/richtree
    kernel/remorph/matcher
    kernel/remorph/text
    kernel/remorph/tokenizer
    kernel/remorph/tokenlogic
    kernel/tarc/iface
    kernel/tarc/markup_zones
    library/cpp/charset
    library/cpp/enumbitset
    library/cpp/getopt
    library/cpp/langmask
    library/cpp/langs
    library/cpp/mime/types
    library/cpp/solve_ambig
)

END()
