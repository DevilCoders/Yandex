PY2_PROGRAM()

OWNER(yanykin)

PY_SRCS(
    __main__.py
)

PEERDIR(
    library/python/resource
    metrika/uatraits/python
)

RESOURCE(
    metrika/uatraits/data/browser.xml browser
    metrika/uatraits/data/extra.xml extra
    metrika/uatraits/data/profiles.xml profiles
)

END()
