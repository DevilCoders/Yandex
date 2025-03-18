PY23_LIBRARY()

OWNER(g:passport_infra)

PEERDIR(
    contrib/python/mock
    library/cpp/tvmauth
    library/cpp/tvmauth/client
)

IF(PYTHON2)
    PEERDIR(contrib/python/enum34)
ENDIF()

PY_SRCS(
    TOP_LEVEL
    ticket_parser2/ticket_parser2_pymodule.pyx
    ticket_parser2/__init__.py
    ticket_parser2/exceptions.py
    ticket_parser2/low_level.py
    ticket_parser2/mock.py
    ticket_parser2/unittest.py
    ticket_parser2/api/__init__.py
    ticket_parser2/api/v1/__init__.py
    ticket_parser2/api/v1/exceptions.py
    ticket_parser2/api/v1/unittest.py
)

ADDINCL(library/cpp/pybind)

END()

RECURSE_FOR_TESTS(
    examples
    mock
    so
    src/ut
)
