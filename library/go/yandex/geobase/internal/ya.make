GO_LIBRARY()

OWNER(
    prime
    g:geotargeting
)

ADDINCL(
    FOR swig contrib/tools/swig/Lib/go
    FOR swig contrib/tools/swig/Lib
)

PEERDIR(
    library/cpp/geobase
    ${GOSTD}/unsafe
    ${GOSTD}/sync
    ${GOSTD}/runtime
    ${GOSTD}/runtime/cgo
)

SRCS(geobase_wrap.swg.cxx)

CGO_SRCS(${BINDIR}/internal.go)

RUN_PROGRAM(
    contrib/tools/swig -c++ -go -cgo -intgosize 64 -o ${BINDIR}/geobase_wrap.swg.cxx -outdir ${BINDIR} ${MODDIR}/geobase.swg
    CWD ${ARCADIA_ROOT}
    IN geobase.swg
    OUT_NOAUTO ${BINDIR}/geobase_wrap.swg.cxx ${BINDIR}/internal.go
)

END()
