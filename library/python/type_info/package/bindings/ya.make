PY_ANY_MODULE(bindings)

OWNER(
    g:yt
)

IF (PYTHON_CONFIG MATCHES "python3" OR USE_SYSTEM_PYTHON MATCHES "3.")
    PYTHON3_MODULE()
ELSE()
    PYTHON2_MODULE()
ENDIF()

PEERDIR(
    library/cpp/type_info
)

SRC(
    ../../bindings.pyx
)

END()
