PROGRAM()

OWNER(g:remorph)

ALLOCATOR(LF)

SRCS(
    main.cpp
    compile_task.cpp
    compiler.cpp
    config.cpp
    config_loader.cpp
    controller.cpp
    gazetteer_pool.cpp
    unit_config.cpp
    var_replacer.cpp
    pb_config.proto
)

PEERDIR(
    contrib/libs/protobuf
    kernel/gazetteer
    kernel/remorph/cascade
    kernel/remorph/common
    kernel/remorph/core
    kernel/remorph/engine/char
    kernel/remorph/info
    kernel/remorph/matcher
    kernel/remorph/proc_base
    kernel/remorph/tokenlogic
    library/cpp/getopt
    library/cpp/regex/pcre
    library/cpp/deprecated/atomic
)

END()
