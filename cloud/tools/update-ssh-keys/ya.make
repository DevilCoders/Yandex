PY2_PROGRAM()

OWNER(bgleb)

PEERDIR(
    contrib/python/cryptography
    contrib/python/requests
    contrib/python/PyJWT
)

PY_MAIN(main)

PY_SRCS(
    TOP_LEVEL
    main.py
)

END()
