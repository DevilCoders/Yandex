PY3_PROGRAM(detect_checker)

OWNER(g:antiadblock)

PEERDIR(
    contrib/python/requests
    contrib/python/selenium
    antiadblock/libs/adb_selenium_lib
    library/python/statface_client
)

PY_SRCS(
    __main__.py
    config.py
    detect_checker_browser.py
    utils/stat_report.py
)

END()
