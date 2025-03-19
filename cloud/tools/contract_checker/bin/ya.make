PY2_PROGRAM(contract-checker)

OWNER(thatguy roboslone)

PY_SRCS(
    MAIN main.py
)

PEERDIR(
    library/python/startrek_python_client
    cloud/tools/contract_checker/contract_checker
    library/python/resource
    search/martylib
)

END()

