PY3_PROGRAM(argus)

OWNER(g:antiadblock)

PEERDIR(
    contrib/python/requests
    contrib/python/pydantic
    contrib/python/python-dotenv
    antiadblock/libs/adb_selenium_lib
    antiadblock/libs/utils
    antiadblock/argus/bin/utils
)

PY_SRCS(
    __main__.py
    config.py
    argus_browser.py
    schemas.py
    browser_pool.py
    browser_group.py
    task_queue.py
    task.py
)

END()
