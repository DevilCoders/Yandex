import os
from gevent import monkey
monkey.patch_all()

from gevent.pywsgi import WSGIServer

from app import app
from app.config import Config

# ToDo replace to startup key

app.config['DEBUG'] = 'True'

# ToDo replace allCA to yav

os.environ['REQUESTS_CA_BUNDLE'] = './allCA.crt'
http_server = WSGIServer(('127.0.0.1', Config.PORT), app)
http_server.serve_forever()
