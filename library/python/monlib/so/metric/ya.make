PY2MODULE(metric)

OWNER(g:solomon msherbakov)

PYTHON2_ADDINCL()

PEERDIR(library/cpp/monlib/metrics)

SRCDIR(library/python/monlib)
SRCS(metric.pyx)

EXPORTS_SCRIPT(metric.exports)

END()
