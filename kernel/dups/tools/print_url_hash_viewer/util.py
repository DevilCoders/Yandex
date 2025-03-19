import flask
import json
import library.python.resource as rs


def load_resources(prefix):
    return rs.iteritems(prefix=prefix, strip_prefix=True)


def make_json_response(data):
    r = flask.make_response(json.dumps(data, ensure_ascii=False))
    r.mimetype = "application/json"
    return r
