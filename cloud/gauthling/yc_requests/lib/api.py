# coding: utf-8

from . import sessions


def request(service_name, credentials, method, url, **kwargs):
    # By using the 'with' statement we are sure the session is closed, thus we
    # avoid leaving sockets open which can trigger a ResourceWarning in some
    # cases, and look like a memory leak in others.
    with sessions.Session(service_name, credentials) as session:
        return session.request(method=method, url=url, **kwargs)


def get(service_name, credentials, url, params=None, **kwargs):
    kwargs.setdefault('allow_redirects', True)
    return request(service_name, credentials, 'get', url, params=params, **kwargs)


def options(service_name, credentials, url, **kwargs):
    kwargs.setdefault('allow_redirects', True)
    return request(service_name, credentials, 'options', url, **kwargs)


def head(service_name, credentials, url, **kwargs):
    kwargs.setdefault('allow_redirects', False)
    return request(service_name, credentials, 'head', url, **kwargs)


def post(service_name, credentials, url, data=None, json=None, **kwargs):
    return request(service_name, credentials, 'post', url, data=data, json=json, **kwargs)


def put(service_name, credentials, url, data=None, **kwargs):
    return request(service_name, credentials, 'put', url, data=data, **kwargs)


def patch(service_name, credentials, url, data=None, **kwargs):
    return request(service_name, credentials, 'patch', url,  data=data, **kwargs)


def delete(service_name, credentials, url, **kwargs):
    return request(service_name, credentials, 'delete', url, **kwargs)
