""" icecream-agent http-client models """
import copy
import logging
import requests


class HTTPClient(object):  # pylint: disable=too-few-public-methods
    """ Base http-client model """

    def __init__(self, host, token=None):
        self.host = host
        self.token = token
        self.headers = {
            'Content-Type': 'application/json',
            'Accept': 'application/json'
        }
        if self.token is not None:
            self.headers['Authorization'] = 'OAuth ' + self.token
        self._log = logging.getLogger()

    def _compile_headers(self, headers):
        """ Safely merge default headers with headers for current request """
        cur_headers = copy.deepcopy(self.headers)
        if headers is not None:
            for key, value in headers.items():
                cur_headers[key] = value
        self._log.debug('Compiled headers from: default: %s, extra: %s, result: %s',
                        self.headers, headers, cur_headers)
        return cur_headers

    def _request(self, endpoint, method='GET', **kwargs):
        """ Just make request and return http answer object """
        kwargs['headers'] = self._compile_headers(kwargs.get('headers'))
        url = self.host + endpoint
        self._log.debug('Making %s request to %s with kwargs: %s',
                        method, url, kwargs)

        if method == 'GET':
            return requests.get(url, **kwargs)
        elif method == 'POST':
            return requests.post(url, **kwargs)
        else:
            raise NotImplementedError


class IceClient(HTTPClient):  # pylint: disable=too-few-public-methods
    """ Icecream api http client """

    def __init__(self, dom0, host, token):
        super(IceClient, self).__init__(host, token)
        self.dom0 = dom0

    def register(self):
        """ Register machine in icecream api """
        endpoint = '/v1/physical'
        payload = self.dom0.to_dict()
        self._log.debug('Request to register ice-agent with payload: %s',
                        payload)
        result = self._request(endpoint, method='POST', json=payload)
        if result.status_code != 200:
            raise IceClientError(
                '%s returned not 200: %s' % (self.host + endpoint, result.status_code),
                result.status_code
            )
        return result


class HTTPClientError(Exception):
    """ Base http client error class """

    def __init__(self, message, code):
        super(HTTPClientError, self).__init__()
        self.message = message
        self.code = code


class IceClientError(HTTPClientError):
    """ Base IceClient """
    pass
