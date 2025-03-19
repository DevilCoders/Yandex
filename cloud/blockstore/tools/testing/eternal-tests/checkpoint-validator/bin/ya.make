PROGRAM(checkpoint-validator)

OWNER(g:cloud-nbs)

SRCS(
    main.cpp
)

PEERDIR(
    cloud/blockstore/client/lib

    cloud/blockstore/tools/testing/eternal-tests/checkpoint-validator/lib
)

END()

