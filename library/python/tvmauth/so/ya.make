PY2MODULE(tvmauth_pymodule)

IF (USE_ARCADIA_PYTHON OR USE_SYSTEM_PYTHON MATCHES "2.7")
    EXPORTS_SCRIPT(tvmauth_pymodule.exports)
ELSE()
    EXPORTS_SCRIPT(tvmauth_pymodule3.exports)
ENDIF()

OWNER(g:passport_infra)

PEERDIR(
    library/cpp/tvmauth/client
)

COPY_FILE(
    ../tvmauth/tvmauth_pymodule.pyx
    tvmauth_pymodule.pyx
)

BUILDWITH_CYTHON_CPP(tvmauth_pymodule.pyx)

END()
