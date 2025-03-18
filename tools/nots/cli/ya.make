PY3_LIBRARY()

OWNER(dankolesnikov)

PY_SRCS(
    main.py
    commands/bundle_webpack.py
    commands/create_node_modules.py
    commands/compile_ts.py
)

PEERDIR(
    build/plugins/lib/nots
)

END()
