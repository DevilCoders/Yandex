OWNER(g:cloud-infra)


PY3_PROGRAM()

PY_SRCS(
    disk_failed_hook.py
    disk_replaced_hook.py
    ecc_failed_hook.py
)

NO_CHECK_IMPORTS()

END()
