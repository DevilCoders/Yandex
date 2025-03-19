#!/usr/bin/env python
from flask import Flask, g, abort, make_response
import json
import logging
import requests
import random
import sys
import socket
import clickhouse

import calculon_config
import clickhouse_config as cc

app = Flask(__name__)
app.config.from_object(__name__)
url_prefix = "/api/v1"
fh = logging.FileHandler('{}'.format(calculon_config.api_log))
if calculon_config.debug:
  fh.setLevel(logging.DEBUG)
else:
  fh.setLevel(logging.WARNING)
app.logger.addHandler(fh)


@app.errorhandler(404)
def not_found(error):
  return make_response(json.dumps({"code": "InvalidNodeName", "message": "NoItemFound"}), 404)


@app.errorhandler(400)
def empty_response(error):
  return make_response(json.dumps({"code": "BadRequest", "message": "BadRequest"}), 400)


@app.errorhandler(405)
def not_found(error):  # pylint: disable=E0102
  return make_response(json.dumps({"code": "InvalidMethod", "message": "MethodNotSupported"}), 405)


@app.errorhandler(500)
def internal_error(error):
  return make_response(json.dumps({"code": "ServerError", "message": "InternalServerError"}), 500)


@app.before_request
def before_request():
  """Make persistent connection to clickhouse server"""
  s = clickhouse.ClickhouseRead(cc.reader, cc.database, cc.user, cc.password)
  s.make_connection()
  s.url = "http://{}:8123/?".format(cc.reader)
  try:
    s.send_request('select 1;')
  except requests.HTTPError as err:
    app.logger.error("Can't connect to {} because of: {}".format(s.url, err))
  if s.established:
    g.db = s
    app.logger.debug('Connection established!')
    app.logger.debug("Request data: {}".format(s.__dict__))
  else:
    abort(500)


def read_data_from_clickhouse(table, fields, tail, last=None):
  """Connect to clickhouse and post data
  :param table: str, table to query
  :param fields: list, columns
  :param tail: str, tail for query"""
  resp = None
  try:
     resp = g.db.select(table, ','.join(fields), tail)
  except Exception as error:
    app.logger.error("Can't read data from clickhouse: {}".format(error))
  if resp:
   if resp['code'] == 200:
     if last:
      return resp['message']['data'][0]
     else:
       return resp['message']['data']
   else:
     abort(404)


@app.route(url_prefix)
def hello():
  message = "Hi! My name is Calculon! I live on {}\nAsk your questions!".format(socket.getfqdn())
  return message


@app.route(url_prefix + '/ping')
def ping():
  """Just answers when it started"""
  return make_response(json.dumps({"code": "OK", "message": "True"}), 200)


@app.route(url_prefix + '/ycu/hosts/<string:hostname>', methods=['GET'])
def get_host_ycu(hostname):
  tail = "WHERE Hostname='{}' order by Time DESC limit 1".format(hostname)
  fields = [el for el in calculon_config.schema['calculon'] if 'YCU' in el]
  table = 'calculon'
  response = None
  try:
    response = read_data_from_clickhouse(table, fields, tail, last=True)
  except Exception as error:
    app.logger.error("No response from clickhouse: {}".format(error))
  if response:
    return json.dumps(response)
  else:
    abort(404)


@app.route(url_prefix + '/iops/hosts/<string:hostname>', methods=['GET'])
def get_host_iops(hostname):
  table = 'disks'
  fields = ['distinct(HCTL)'] + [el for el in calculon_config.schema[table] if el.startswith('iops')]
  tail = "WHERE Hostname='{}' order by Time DESC".format(hostname)
  response = None
  try:
    response = read_data_from_clickhouse(table, fields, tail)
  except Exception as error:
    app.logger.error("No response from clickhouse: {}".format(error))
  if response:
    return json.dumps(response)
  else:
    abort(404)


@app.route(url_prefix + '/ycu/hwid/<string:hwid>')
def get_hwid_ycu(hwid):
  determinator = random.randint(0, 100)
  table = 'calculon'
  fields = ['toString(quantileDeterministic(.9)(toFloat64(YCU), toInt32({}))) as YCU'.format(determinator),
            'toString(quantileDeterministic(.9)(toFloat64(VCPU_YCU), toInt32({}))) as VCPU_YCU'.format(determinator)]
  tail = "WHERE HWID='{}' group by Time order by Time DESC limit 1".format(hwid)
  response = None
  try:
    response = read_data_from_clickhouse(table, fields, tail, last=True)
  except Exception as error:
    app.logger.error("No response from clickhouse: {}".format(error))
  if response:
    return json.dumps(response)
  else:
    abort(404)


@app.route(url_prefix + '/iops/hosts/<string:hostname>/<string:hctl>')
def get_particular_disk_iops_information(hostname, hctl):
  fields = [el for el in calculon_config.schema['disks'] if el.startswith('iops')]
  table = "(select * from disks where Hostname='{}' and HCTL='{}'".format(hostname, hctl)
  tail = "order by Time DESC limit 1)"
  response = None
  try:
    response = read_data_from_clickhouse(table, fields, tail, last=True)
  except Exception as error:
    app.logger.error("No response from clickhouse: {}".format(error))
  if response:
    return json.dumps(response)
  else:
    abort(404)


@app.route(url_prefix + '/iops/<string:model>')
def get_model_iops(model):
  fields = [el for el in calculon_config.schema['disks'] if el.startswith('iops')]
  table = 'disks'
  tail = "WHERE Model='{}' order by Time DESC limit 1".format(model)
  response = None
  try:
    response = read_data_from_clickhouse(table, fields, tail, last=True)
  except Exception as error:
    app.logger.error("No response from clickhouse: {}".format(error))
  if response:
    return json.dumps(response)
  else:
    abort(404)


if __name__ == '__main__':
  app.debug = True
  app.run()
