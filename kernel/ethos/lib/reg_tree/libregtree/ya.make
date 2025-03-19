PY2MODULE(libregtree)

OWNER(alex-sh)

PYTHON2_ADDINCL()

SRCS(
    regtreemodule.cpp
)

PEERDIR(
    kernel/ethos/lib/reg_tree
    library/cpp/pybind
)

END()
