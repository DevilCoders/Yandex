PY3_PROGRAM()

OWNER(lunin-dv)

PEERDIR(
    contrib/python/numpy
    contrib/python/pandas
    contrib/python/scipy
    contrib/python/dateutil
    contrib/python/PyMySQL
    library/python/vault_client
    yt/python/client_with_rpc
)

PY_SRCS(   # Макрос исходных файлов модуля
   __main__.py
)


END()
