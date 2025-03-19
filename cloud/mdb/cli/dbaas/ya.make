OWNER(g:mdb)

PY3_PROGRAM(dbaas)

PY_SRCS(
	MAIN dbaas.py
)

PEERDIR(
    cloud/mdb/cli/dbaas/commands
    cloud/mdb/cli/dbaas/internal

    contrib/python/click
)

END()

RECURSE(
    commands
    internal
    tests
)
