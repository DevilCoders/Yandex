PY23_LIBRARY()

OWNER(
    dskut
    g:mail
)

PY_SRCS(
    __init__.py
    pyremock.py
    matchers/__init__.py
    matchers/is_json_serialized.py
    matchers/is_date_close_to.py
)

PEERDIR(
    contrib/python/decorator
    contrib/python/PyHamcrest
    contrib/python/retrying
    contrib/python/tornado/tornado-4
)

END()
