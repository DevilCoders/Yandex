PY23_LIBRARY()

OWNER(g:passport_infra)

PEERDIR(
    contrib/python/mock
    library/python/deprecated/ticket_parser2
)

PY_SRCS(
    TOP_LEVEL
    ticket_parser2_mock/__init__.py
    ticket_parser2_mock/api/__init__.py
    ticket_parser2_mock/api/v1/__init__.py
    ticket_parser2_mock/api/v1/tvm_client.py
)

END()
