#!/usr/bin/env python
from flask import Flask, request, g, abort, make_response
import json
import logging
import requests
import socket
import datetime
import clickhouse
import clickhouse_config as cc
import calculon_config

app = Flask(__name__)
app.config.from_object(__name__)
url_prefix = "/api/receiver"
fh = logging.FileHandler('{}'.format(calculon_config.collector_log))
if calculon_config.debug:
  fh.setLevel(logging.DEBUG)
else:
  fh.setLevel(logging.WARNING)
app.logger.addHandler(fh)


def get_delta():
  now = datetime.datetime.now()
  update = datetime.datetime.strptime('01-01-1970', "%d-%m-%Y")
  delta = now - update
  return delta.days


@app.errorhandler(404)
def not_found(error):
  return make_response(json.dumps({"code": "InvalidNodeName", "message": "NoItemFound"}), 404)


@app.errorhandler(400)
def empty_response(error):
  return make_response(json.dumps({"code": "BadRequest", "message": "BadRequest"}), 400)


@app.errorhandler(405)
def not_found(error):  # pylint: disable=E0102
  return make_response(json.dumps({"code": "InvalidMethod", "message": "MethodNotSUpported"}), 405)


@app.errorhandler(500)
def internal_error(error):
  return make_response(json.dumps({"code": "ServerError", "message": "InternalServerError"}), 500)


@app.before_request
def before_request():
  """Make persistent connection to clickhouse server"""

  resp = None
  s = clickhouse.ClickhouseWrite(cc.writer, cc.database, cc.user, cc.password)
  s.make_connection()
  s.url = "http://{}:8123/?".format(cc.writer)
  try:
    resp = s.send_request('select 1;')
  except requests.HTTPError as err:
    app.logger.error("Can't connect to {} because of: {}".format(s.url, err))
  if s.established:
    g.db = s
    app.logger.debug('Connection established!')
    app.logger.debug("Write object: {}".format(s.__dict__))
  else:
    abort(500)

def send_data_to_clickhouse(table, data):
  """Connect to clickhouse and post data
  :param table: str, table to insert
  :param data: str, positional substitution"""
  try:
     resp = g.db.insert(table, data)
  except Exception as error:
    app.logger.error("Can't send data to clickhouse: {}".format(error))
  if resp:
    if resp['code'] == 200:
      return make_response(json.dumps({"code": "OK", "message": "OK"}), 200)
    else:
      abort(resp['code'])


#@app.teardown_request
#def teardown_request(exception):
#  if hasattr(g, 'db'):
#    g.db.close()
#

@app.route(url_prefix)
def hello():
  message = "Hi! My name is Calculon! I live on {}".format(socket.getfqdn())
  return message


@app.route(url_prefix + '/ping')
def ping():
  """Just answers when it started"""
  return make_response(json.dumps({"code": "OK", "message": "True"}), 200)


@app.route(url_prefix + '/upload/calculon/', methods=['POST'])
def receive_ycu():
  table = 'calculon'
  data = request.json
  try:
    data = json.loads(data)
    app.logger.debug('INCOMING DATA: {}'.format(data['data']))
  except ValueError:  # json.loads in python2 raises ValueError
    abort(400)
  return send_data_to_clickhouse(table, data['data'])


@app.route(url_prefix + '/upload/disks/', methods=['POST'])
def receive_host_iops():
  table = 'disks'
  data = request.json
  try:
    data = json.loads(data)
    app.logger.debug('INCOMING DATA: {}'.format(data['data']))
  except ValueError:  # json.loads in python2 raises ValueError
    abort(400)
  return send_data_to_clickhouse(table, data['data'])


if __name__ == '__main__':
  app.debug = True
  app.run()
