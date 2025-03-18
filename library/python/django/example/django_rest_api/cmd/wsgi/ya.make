PY3_PROGRAM(app)

OWNER(lorekhov)

NO_CHECK_IMPORTS()

PEERDIR(
    library/python/django/example/django_rest_api
    library/python/gunicorn
)

PY_SRCS(
    app.py
)

PY_MAIN(library.python.django.example.django_rest_api.cmd.wsgi.app)


END()
