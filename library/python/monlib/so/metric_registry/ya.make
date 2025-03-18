PY2MODULE(metric_registry)

OWNER(g:solomon msherbakov)

PYTHON2_ADDINCL()

PEERDIR(library/cpp/monlib/metrics)

SRCDIR(library/python/monlib)
SRCS(metric_registry.pyx)

EXPORTS_SCRIPT(metric_registry.exports)

END()
