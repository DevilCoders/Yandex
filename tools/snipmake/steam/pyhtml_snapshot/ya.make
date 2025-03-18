PY2MODULE(pyhtml_snapshot 1 0 PREFIX lib)

OWNER(g:snippets)

PYTHON2_ADDINCL()

PEERDIR(
    dict/recognize/docrec
    library/cpp/charset
    tools/snipmake/steam/html_snapshot
    tools/snipmake/steam/pyhtml_snapshot/protos
)

SRCS(
    py_api.cpp
    html_snapshot.swg
)

END()
