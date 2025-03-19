PY3_PROGRAM(iam-codegen)

OWNER(g:cloud-iam)

PY_SRCS(
    __main__.py
    libcodegen.py
)

PEERDIR(
    contrib/python/Jinja2
    contrib/python/PyYAML
    contrib/python/simplejson
)

END()
