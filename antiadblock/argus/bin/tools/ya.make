PY3_PROGRAM(builder)

OWNER(g:antiadblock)

PEERDIR(
    sandbox/projects/antiadblock/aab_automerge_ext
    sandbox/projects/antiadblock/aab_argus_utils
    sandbox/sdk2
)

PY_SRCS(
    MAIN builder.py
)

END()
