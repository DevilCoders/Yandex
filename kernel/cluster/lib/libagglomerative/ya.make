PY2MODULE(libagglomerative)

OWNER(alex-sh)

PYTHON2_ADDINCL()

SRCS(
    agglomerativemodule.cpp
)

PEERDIR(
    kernel/cluster/lib/agglomerative
    library/cpp/pybind
)

END()
