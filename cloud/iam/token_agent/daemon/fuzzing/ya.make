OWNER(g:cloud-iam)

FUZZ()
#PROGRAM(fuzz-debug)

PEERDIR(
    ADDINCL cloud/iam/token_agent/daemon/lib
	contrib/libs/jwt-cpp
)

SRCS(
    main.cpp
)

END()
