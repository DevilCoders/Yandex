OWNER(g:go-library)

RECURSE(
    blockcodecs
    certifi
    cgosem
    core
    discreterand
    godoc
    httputil
    k8s
    masksecret
    maxprocs
    ptr
    refactor
    slices
    test
    units
    urlutil
    valid
    vanity
    x
    yandex
    yatool
    yo
    yoimports
    yolint
)

IF (OS_LINUX)
    RECURSE(squashfs)
ENDIF()