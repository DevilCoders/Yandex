PROGRAM(solver)

OWNER(
    grmammaev
    g:clustermaster
    g:jupiter
)

SET_APPEND(CFLAGS -DCOMMUNISM_SOLVER)

PEERDIR(
    library/cpp/getoptpb
    library/cpp/logger
    library/cpp/svnversion
    library/cpp/terminate_handler
    tools/clustermaster/proto
    tools/clustermaster/communism/core
    tools/clustermaster/communism/util
    library/cpp/deprecated/atomic
)

SRCS(
    request.cpp
    batch.cpp
    solver.cpp
    http.cpp
    http_static.cpp
)

ARCHIVE(
    NAME http_static.inc
    http_static/style.css
)

END()
