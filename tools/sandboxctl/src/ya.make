PY2_LIBRARY()
OWNER(dmtrmonakhov)

PEERDIR(
    contrib/python/requests
    devtools/ya/core
    devtools/ya/test/dependency/sandbox_storage
    devtools/ya/test/util
    devtools/ya/yalibrary/display
    devtools/ya/yalibrary/tools
    devtools/ya/yalibrary/yandex/sandbox
    devtools/ya/yalibrary/vcs
    library/python/oauth
    library/python/svn_version
    contrib/python/lxml
    sandbox/common
)

PY_SRCS(
    __init__.py
    main.py
    ui.py
)
END()

