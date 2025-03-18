PY2MODULE(encoder)

OWNER(g:solomon msherbakov)

PYTHON2_ADDINCL()

PEERDIR(
    library/cpp/monlib/encode/spack
    library/cpp/monlib/encode/json
    library/cpp/monlib/encode/unistat
)

SRCDIR(library/python/monlib)
SRCS(encoder.pyx)

EXPORTS_SCRIPT(encoder.exports)

END()
