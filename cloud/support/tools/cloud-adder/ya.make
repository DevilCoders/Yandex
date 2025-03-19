PY3_PROGRAM(cloud-adder)

OWNER(wunderkater)

PEERDIR(
    contrib/python/certifi
    contrib/python/tqdm
    contrib/python/Telethon
    contrib/python/httpx
)

PY_SRCS(
    MAIN
    cloud-adder.py
)

END()
