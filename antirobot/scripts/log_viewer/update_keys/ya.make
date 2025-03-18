OWNER(g:antirobot)

PY3_PROGRAM()

PY_SRCS(
    TOP_LEVEL
    main.py
)

PY_MAIN(main)

PEERDIR(
    devtools/ya/yalibrary/yandex/sandbox
)

END()
