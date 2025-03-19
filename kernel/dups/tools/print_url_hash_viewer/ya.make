OWNER(swarmer)

PY2_PROGRAM()

PY_SRCS(
    TOP_LEVEL
    __main__.py
    app.py
    urlnorm.pyx
    util.py
)

PEERDIR(
    kernel/dups
    contrib/python/Flask
    contrib/python/Jinja2
    contrib/python/futures
    library/python/resource
)

RESOURCE(
    res/style.css /res/style.css
    templates/base.html /jinja/base.html
    templates/main.html /jinja/main.html
)

END()
