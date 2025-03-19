import json
import datetime
import ipaddress
import time
import uuid
import platform

from functools import wraps

from requests.sessions import Session

from urllib.parse import urlparse

from yc_common import logging, metrics, constants

from .exceptions import HttpError
from .utils import FQName, to_json


log = logging.get_logger(__name__)

contrail_api_request_latency = metrics.Metric(
    metrics.MetricTypes.HISTOGRAM,
    "contrail_api_request_latency",
    ["method", "path", "status_code", "status"],
    "Contrail API requests latency histogram.",
    buckets=metrics.time_buckets_ms)


def contrail_error_handler(f):
    """Handle HTTP errors returned by the API server
    """
    @wraps(f)
    def wrapper(*args, **kwargs):
        try:
            return f(*args, **kwargs)
        except HttpError as e:
            # Replace message by details to provide a
            # meaningful message
            if e.details:
                e.message, e.details = e.details, e.message
                e.args = ("{} (HTTP {})".format(e.message, e.http_status),)
            raise
    return wrapper


class _JSONEncoder(json.JSONEncoder):

    def default(self, o):
        if isinstance(o, datetime.datetime):
            return o.isoformat()
        elif isinstance(o, uuid.UUID):
            return str(o)
        elif isinstance(o, ipaddress._IPAddressBase):
            return str(o)
        return super().default(o)


class ContrailAPISession(object):
    user_agent = "contrail-api-cli"

    def __init__(self,
                 host: str = "localhost",
                 port: int = 8082,
                 protocol: str = "http",
                 request_id: str = None,
                 operation_id: str = None,
                 ):
        self.host = host
        self.port = port
        self.protocol = protocol
        self._json = _JSONEncoder()
        self.session = Session()
        self.session.headers = {
            "User-Agent": self.user_agent,
            "Content-Type": "application/json",
            "X-Contrail-Useragent": "{}:{}".format(platform.node(), "compute-contrail-api"),
        }
        if request_id is not None:
            self.session.headers["X-Request-ID"] = request_id
        if operation_id is not None:
            self.session.headers["X-Operation-ID"] = operation_id

    @property
    def adapters(self):
        return self.session.adapters

    @adapters.setter
    def adapters(self, value):
        self.session.adapters = value

    def mount(self, scheme, adapter):
        self.session.mount(scheme, adapter)

    @property
    def user(self):
        if hasattr(self.auth, 'username'):
            return self.auth.username
        else:
            return 'unknown'

    @property
    def base_url(self):
        return "{}://{}:{}".format(self.protocol, self.host, self.port)

    def make_url(self, uri):
        return self.base_url + uri

    def request(self, url, method, json=None, raise_exc=True, **kwargs):
        headers = kwargs.setdefault('headers', dict())

        if json is not None:
            headers.setdefault('Content-Type', 'application/json')
            kwargs['data'] = self._json.encode(json)

        log.debug("API call: %s %s", method, url)

        request_start_time = time.monotonic()
        # Common REST-api path is /entity/... e.g. /virtual-DNS/<id>
        target = urlparse(url).path.split('/')[1]
        try:
            resp = self.session.request(method, url, **kwargs)
        except Exception as e:
            request_time = time.monotonic() - request_start_time
            log.error("[rt=%.2f] API call failed with error: %s", request_time, e)
            contrail_api_request_latency.labels(method, target, 0, "exception_raised: " + type(e).__name__).observe(request_time * constants.SECOND_MILLISECONDS)
            raise

        request_time = time.monotonic() - request_start_time
        log.debug("[rt=%.2f] API call has completed with %s status code.", request_time, resp.status_code)
        contrail_api_request_latency.labels(method, target, resp.status_code, "complete").observe(request_time * constants.SECOND_MILLISECONDS)
        if raise_exc and resp.status_code >= 400:
            req_id = resp.headers.get("x-openstack-request-id")

            _kwargs = {
                "http_status": resp.status_code,
                "response": resp,
                "method": method,
                "url": url,
                "request_id": req_id,
            }
            if "retry-after" in resp.headers:
                _kwargs["retry_after"] = resp.headers["retry-after"]

            content_type = resp.headers.get("Content-Type", "")
            if content_type.startswith("application/json"):
                try:
                    body = resp.json()
                except ValueError:
                    pass
                else:
                    if isinstance(body, dict) and isinstance(body.get("error"), dict):
                        error = body["error"]
                        _kwargs["message"] = error.get("message")
                        _kwargs["details"] = error.get("details")
            elif content_type.startswith("text/"):
                _kwargs["details"] = resp.text

            raise HttpError(**_kwargs)

        return resp

    @contrail_error_handler
    def get_json(self, url, **kwargs):
        return self.get(url, params=kwargs).json()

    @contrail_error_handler
    def post_json(self, url, data, cls=None, **kwargs):
        """
        POST data to the api-server

        :param url: resource location (eg: "/type/uuid")
        :type url: str
        :param cls: JSONEncoder class
        :type cls: JSONEncoder
        """
        kwargs['data'] = to_json(data, cls=cls)
        return self.post(url, **kwargs).json()

    @contrail_error_handler
    def put_json(self, url, data, cls=None, **kwargs):
        """
        PUT data to the api-server

        :param url: resource location (eg: "/type/uuid")
        :type url: str
        :param cls: JSONEncoder class
        :type cls: JSONEncoder
        """
        kwargs['data'] = to_json(data, cls=cls)
        return self.put(url, **kwargs).json()

    def head(self, url, **kwargs):
        return self.request(url, 'HEAD', **kwargs)

    def get(self, url, **kwargs):
        return self.request(url, 'GET', **kwargs)

    def post(self, url, **kwargs):
        return self.request(url, 'POST', **kwargs)

    def put(self, url, **kwargs):
        return self.request(url, 'PUT', **kwargs)

    @contrail_error_handler
    def delete(self, url, **kwargs):
        return self.request(url, 'DELETE', **kwargs)

    def patch(self, url, **kwargs):
        return self.request(url, 'PATCH', **kwargs)

    def fqname_to_id(self, fq_name, type):
        """
        Return uuid for fq_name

        :param fq_name: resource fq name
        :type fq_name: FQName
        :param type: resource type
        :type type: str

        :rtype: UUIDv4 str
        :raises HttpError: fq_name not found
        """
        data = {
            "type": type,
            "fq_name": list(fq_name)
        }
        return self.post_json(self.make_url("/fqname-to-id"), data)["uuid"]

    def id_to_fqname(self, uuid, type=None):
        """
        Return fq_name and type for uuid

        If `type` is provided check that uuid is actually
        a resource of type `type`. Raise HttpError if it's
        not the case.

        :param uuid: resource uuid
        :type uuid: UUIDv4 str
        :param type: resource type
        :type type: str

        :rtype: dict {'type': str, 'fq_name': FQName}
        :raises HttpError: uuid not found
        """
        data = {
            "uuid": uuid
        }
        result = self.post_json(self.make_url("/id-to-fqname"), data)
        result['fq_name'] = FQName(result['fq_name'])
        if type is not None and not result['type'].replace('_', '-') == type:
            raise HttpError('uuid {} not found for type {}'.format(uuid, type), http_status=404)
        return result

    def add_ref(self, r1, r2, attr=None):
        self._ref_update(r1, r2, 'ADD', attr)

    def remove_ref(self, r1, r2):
        self._ref_update(r1, r2, 'DELETE')

    def _ref_update(self, r1, r2, action, attr=None):
        data = {
            'type': r1.type,
            'uuid': r1.uuid,
            'ref-type': r2.type,
            'ref-uuid': r2.uuid,
            'ref-fq-name': list(r2.fq_name),
            'operation': action,
            'attr': attr
        }
        return self.post_json(self.make_url("/ref-update"), data)

    def search_kv_store(self, key):
        """Search for a key in the key-value store.
        :param key: string
        :rtype: string
        """
        data = {
            'operation': 'RETRIEVE',
            'key': key
        }
        return self.post_json(self.make_url("/useragent-kv"), data)['value']

    def get_kv_store(self):
        """Retrieve all key-value store elements.

        :rtype: [{key, value}] where key and value are strings.
        """
        data = {
            'operation': 'RETRIEVE',
            'key': None
        }
        return self.post_json(self.make_url("/useragent-kv"), data)['value']

    def add_kv_store(self, key, value):
        """Add a key-value store entry.

        :param key: string
        :param value: string
        """
        data = {
            'operation': 'STORE',
            'key': key,
            'value': value
        }
        return self.post(self.make_url("/useragent-kv"), data=to_json(data)).text

    def remove_kv_store(self, key):
        """Remove a key-value store entry.

        :param key: string
        """
        data = {
            'operation': 'DELETE',
            'key': key
        }
        return self.post(self.make_url("/useragent-kv"), data=to_json(data)).text
