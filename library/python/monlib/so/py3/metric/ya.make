PY3MODULE(metric)

OWNER(g:solomon msherbakov)

PYTHON3_ADDINCL()

PEERDIR(library/cpp/monlib/metrics)

SRCDIR(library/python/monlib)
SRCS(metric.pyx)

EXPORTS_SCRIPT(metric.exports)

END()
