PY2_LIBRARY()

OWNER(thatguy roboslone)

PY_SRCS(
    __init__.py
)

PEERDIR(
    library/python/startrek_python_client
    cloud/tools/contract_checker/contract_checker_protoc
    library/python/resource
    search/martylib
)

RESOURCE(
    message.txt /contract_checker/message.txt
)

END()
