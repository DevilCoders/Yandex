PY2MODULE(cookiemy)

EXPORTS_SCRIPT(cookiemy.exports)

OWNER(
    g:passport_python
)

PEERDIR(
    library/python/cookiemy/srcs
)

COPY_FILE(
    ../cookiemy.pyx
    cookiemy.pyx
)

BUILDWITH_CYTHON_CPP(cookiemy.pyx)

END()
