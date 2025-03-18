PY3_LIBRARY()

OWNER(g:antiadblock)

PEERDIR(
    contrib/python/pydantic
    contrib/python/boto3
    contrib/python/botocore
    contrib/python/Pillow
    contrib/python/requests
    contrib/python/selenium
)

PY_SRCS(
    __init__.py
    browsers/__init__.py
    browsers/base_browser.py
    browsers/extensions.py
    browsers/chrome.py
    browsers/firefox.py
    browsers/opera.py
    utils/screenshot.py
    utils/exceptions.py
    config.py
    schemas.py
)

END()
