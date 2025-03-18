OWNER(g:antirobot)

PY2_LIBRARY()

PY_SRCS(
    filter_yandex.py
    flash_uid.py
    ip_utils.py
    join_files.py
    unquote_safe.py
    crc64.py
    log_setup.py
    spravka2.pyx
)

PEERDIR(
    antirobot/lib
)

END()
