OWNER(g:antirobot)

RECURSE_ROOT_RELATIVE(
    antirobot/idl/python
    library/cpp/eventlog/proto/python
)


PY23_LIBRARY()

PEERDIR(
    antirobot/idl
    )

PY_SRCS(
    event.py
    )

END()
