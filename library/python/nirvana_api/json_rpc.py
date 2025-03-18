import uuid
import json
import six

import logging
logger = logging.getLogger(__name__)


class DefaultEncoder(json.JSONEncoder):

    def default(self, o):
        if hasattr(o, '__json__'):
            return o.__json__()
        else:
            return self._normalize(o.__dict__)

    @staticmethod
    def _normalize(o_dict):
        """
        Remove underscores which introduced because of collisions with builtins, i.e self.from_ -> self.from
        """
        return {key.rstrip('_'): value for key, value in six.iteritems(o_dict)}


class Obj(object):

    def __init__(self, d):
        for a, b in d.items():
            if isinstance(b, (list, tuple)):
                setattr(self, a, [Obj(x) if isinstance(x, dict) else x for x in b])
            else:
                setattr(self, a, Obj(b) if isinstance(b, dict) else b)

    def __repr__(self):
        return repr(self.__dict__)

    def __str__(self):
        return str(self.__dict__)

    def __getitem__(self, attr):
        if hasattr(self, attr):
            return getattr(self, attr)
        else:
            raise KeyError(attr)

    def __contains__(self, attr):
        return hasattr(self, attr)

    def get(self, attr, default=None):
        return getattr(self, attr, default)

    def __iter__(self):
        return six.iteritems(self.__dict__)


def _make_obj(d):
    return Obj(d)


def serialize(method, params, request_id=None):
    req = {
        'jsonrpc': '2.0',
        'id':  request_id if request_id else str(uuid.uuid4()),
        'method': method,
        'params': params
    }
    return json.dumps(req, cls=DefaultEncoder)


def serialize_batch(method, params, request_id=None):
    if not request_id:
        request_id = str(uuid.uuid4())
    req = [{
        'jsonrpc': '2.0',
        'id':  request_id + '-' + str(i),
        'method': method,
        'params': param
    } for i, param in enumerate(params)]
    return json.dumps(req, cls=DefaultEncoder)


class RPCException(Exception):
    pass


def deserialize(response):
    try:
        response_obj = _make_obj(json.loads(response))
    except ValueError as e:
        logger.error('Invalid response: {}'.format(response))
        raise e
    # assert response_obj.jsonrpc == '2.0'
    # assert response_obj.id == request_id
    if hasattr(response_obj, 'error'):
        error = response_obj.error
        raise RPCException(error.code, error.message, getattr(error, 'data', ''))
    return response_obj.result


def deserialize_batch(response, base_request_id):
    try:
        ans = json.loads(response)
        if isinstance(ans, list):
            response_objs = [_make_obj(o) for o in ans]
        else:
            response_objs = [_make_obj(ans)]
    except ValueError as e:
        logger.error('Invalid response: {}'.format(response))
        raise e

    for o in response_objs:
        if not o.id.startswith(base_request_id + '-'):
            raise Exception('request id {} doesnt start with {}'.format(o.id, base_request_id + '-'))
        (base_id, sub_id) = o.id.rsplit('-', 1)
        setattr(o, 'sub_id', int(sub_id))
    return response_objs
