PROGRAM()

OWNER(
    g:wizard
)

PEERDIR(
    search/wizard
    library/cpp/eventlog
    search/begemot/status
    tools/printwzrd/lib
    library/cpp/threading/serial_postprocess_queue
    tools/wizard_yt/shard_packer
    mapreduce/yt/interface
    mapreduce/yt/client
    search/wizard/common
    search/wizard/face
    search/fields
    web/daemons/wizard/wizard_main
    search/wizard/core
    search/wizard/config
    tools/wizard_yt/reducer_common
    search/idl
)

ALLOCATOR(B)

SRCS(
    main.cpp
    processrow_task.cpp
)

END()
