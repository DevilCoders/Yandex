OWNER(g:antirobot)

RECURSE(
    precalc
    slow_search
    update_keys
)

PY2_PROGRAM(log_viewer_uwsgi)

PY_SRCS(
    TOP_LEVEL
    main.py
)

PY_MAIN(main)

PEERDIR(
    contrib/python/uwsgi
    antirobot/scripts/log_viewer/app
)

END()
