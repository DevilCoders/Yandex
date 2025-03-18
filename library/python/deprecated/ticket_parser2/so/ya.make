PY2MODULE(ticket_parser2_pymodule)

IF (USE_ARCADIA_PYTHON OR USE_SYSTEM_PYTHON MATCHES "2.7")
    EXPORTS_SCRIPT(ticket_parser2_pymodule.exports)
ELSE()
    EXPORTS_SCRIPT(ticket_parser2_pymodule3.exports)
ENDIF()

OWNER(g:passport_infra)

PEERDIR(
    library/cpp/tvmauth
    library/cpp/tvmauth/client
)

COPY_FILE(
    ../ticket_parser2/ticket_parser2_pymodule.pyx
    ticket_parser2_pymodule.pyx
)

BUILDWITH_CYTHON_CPP(ticket_parser2_pymodule.pyx)

END()
