PROGRAM()

OWNER(
    tddy
    ya-spe
    g:wizard
)

PEERDIR(
    kernel/gazetteer
    library/cpp/eventlog
    library/cpp/streams/factory
    library/cpp/threading/serial_postprocess_queue
    search/fields
    search/idl
    search/wizard
    search/wizard/common/core
    search/wizard/config
    search/wizard/core
    search/wizard/face
    search/wizard/remote
    tools/printwzrd/lib
    ysite/yandex/reqdata
)

#use lfalloc explicitly
ALLOCATOR(LF)

SRCS(
    printwzrd.cpp
    process_print.cpp
    printwzrd_task.cpp
)

# TODO: actualize rules or remove undesirable dependencies
# CHECK_DEPENDENT_DIRS(
#     DENY
#     ERROR
#     quality
# )

END()

RECURSE(
    configs
)
