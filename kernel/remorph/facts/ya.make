LIBRARY()

OWNER(g:remorph)

SRCS(
    facts.cpp
    fact.cpp
    factprocessor.cpp
    facttype.cpp
    type_database.cpp
    factmeta.proto
)

PEERDIR(
    contrib/libs/protobuf
    kernel/gazetteer
    kernel/geograph
    kernel/remorph/cascade
    kernel/remorph/common
    kernel/remorph/core
    kernel/remorph/engine/char
    kernel/remorph/input
    kernel/remorph/matcher
    kernel/remorph/misc/proto_parser
    kernel/remorph/proc_base
    kernel/remorph/tokenizer
    kernel/remorph/tokenlogic
    library/cpp/json
    library/cpp/json/writer
    library/cpp/protobuf/json
    library/cpp/solve_ambig
)

END()
