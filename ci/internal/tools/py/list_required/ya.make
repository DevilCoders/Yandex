PY3_PROGRAM()

OWNER(g:ci)

PY_MAIN(ci.internal.tools.py.list_required.main)

PY_SRCS(
    main.py
)

PEERDIR(
    sandbox/projects/common/build
)

END()
