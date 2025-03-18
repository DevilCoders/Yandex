PROGRAM()

OWNER(g:wizard)

PEERDIR(
    tools/wizard_yt/shard_packer
    tools/wizard_yt/reducer_common
    library/cpp/getopt
    mapreduce/yt/interface
    mapreduce/yt/client
    library/cpp/digest/md5
    library/cpp/string_utils/base64
)

SRCS(
    main.cpp
)

END()
