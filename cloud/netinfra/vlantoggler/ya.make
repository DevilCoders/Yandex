PY3_PROGRAM(vlantoggler)

OWNER(horseinthesky)

PY_SRCS(
  TOP_LEVEL
  config.py
  run.py
  app/__init__.py
  app/api.py
  app/forms.py
  app/netbox_helper.py
  app/views.py
  app/vlantoggler.py
)

PY_MAIN(
  run
)

PEERDIR(
  contrib/python/Flask
  contrib/python/Flask-Bootstrap
  contrib/python/Flask-WTF
  contrib/python/Flask-RESTful
  contrib/python/xmltodict
  contrib/python/pynetbox
  contrib/python/ncclient
)


END()
