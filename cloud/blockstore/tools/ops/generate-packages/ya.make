PY3_PROGRAM(generate-packages)

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/Jinja2

    library/python/resource
)

RESOURCE(
    pkg.json.jinja2 pkg.json.jinja2
)

END()
