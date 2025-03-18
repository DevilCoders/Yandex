PY2_PROGRAM(stub_generator)

OWNER(g:antiadblock)

PY_SRCS(
    __main__.py
    ammo_generator.py
)

PEERDIR(
    antiadblock/cryprox/cryprox
    antiadblock/libs/decrypt_url
    contrib/python/Jinja2
    contrib/python/matplotlib
    contrib/python/numpy
)

END()
