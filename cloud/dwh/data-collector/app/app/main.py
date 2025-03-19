from contextlib import closing
from json import JSONDecodeError

from flask import Flask, abort
from flask import request, jsonify
import json
import psycopg2
import os
from datetime import datetime
import email_normalize
import ast


app = Flask(__name__)

DB_NAME = os.environ['YC_DWH_DATABASE']
DB_HOST = os.environ['YC_DWH_HOST']
DB_PORT = os.environ['YC_DWH_PORT']
DB_USER = os.environ['YC_DWH_USER']
DB_PASSWORD = os.environ["YC_DWH_PASSWORD"]


def parse_forms_data(data: dict):
    try:
        for key in data.keys():
            data[key] = json.loads(data[key])
    except JSONDecodeError:
        abort(400, "Invalid forms data format")
    return json.dumps(data)


def parse_forms_headers(data: dict):
    data["X-Cookies"] = ast.literal_eval(data["X-Cookies"])
    return json.dumps(data)


def parse_email(email):
    try:
        return email_normalize.normalize(email).normalized_address
    except ValueError:
        abort(400, "Email is not valid")


def require_email_param(request):
    email = request.json.get("email")
    if not email:
        abort(400, "Email is not provided")


def require_json(request):
    if not request.is_json:
        abort(400, "Request Content-Type must be application/json")


@app.route('/api/v1/forms-data', methods=['GET', 'POST', 'PUT'])
def forms_data():
    if not request.method == 'GET':
        data = parse_forms_data(dict(request.form))
        headers = parse_forms_headers(dict(request.headers))
        with closing(psycopg2.connect(dbname=DB_NAME, user=DB_USER, port=DB_PORT,
                                      password=DB_PASSWORD, host=DB_HOST)) as conn:
            with conn.cursor() as cursor:
                conn.autocommit = True
                cursor.execute('insert into raw.frm_registration(data, headers, _insert_dttm)'
                               'values (%s, %s, %s)', (data, headers, datetime.utcnow()))
    return "OK"


@app.route('/api/v1/subscribe-news', methods=['GET', 'POST', 'PUT'])
def subscribe_news():
    if not request.method == 'GET':
        require_json(request)
        require_email_param(request)
        email = parse_email(request.json["email"])
        with closing(psycopg2.connect(dbname=DB_NAME, user=DB_USER, port=DB_PORT,
                                      password=DB_PASSWORD, host=DB_HOST)) as conn:
            with conn.cursor() as cursor:
                conn.autocommit = True
                cursor.execute('insert into ods.stg_site_news_subscribed_email(email, _insert_dttm)'
                               'values (%s, %s)', (email, datetime.utcnow()))
    return "OK"


@app.errorhandler(404)
def page_not_found(e):
    return jsonify(error=404, text=str(e)), 404


@app.errorhandler(403)
def forbidden(e):
    return jsonify(error=403, text=str(e)), 403


@app.errorhandler(410)
def gone(e):
    return jsonify(error=410, text=str(e)), 410


@app.errorhandler(500)
def internal_server_error(e):
    return jsonify(error=500, text=str(e)), 500


@app.errorhandler(400)
def bad_request(e):
    return jsonify(error=400, text=str(e)), 400
