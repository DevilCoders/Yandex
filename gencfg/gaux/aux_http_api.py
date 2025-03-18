# coding: utf8
import os
import time
import json
import requests
import contrib.retrying as retrying


class HttpApi(object):
    METHODS = {
        'GET': requests.get,
        'POST': requests.post,
        'PUT': requests.put,
        'DELETE': requests.delete,
    }

    def __init__(self, base_url=None, oauth_token=None, disable_warnings=True):
        self.base_url = base_url
        self.oauth_token = oauth_token

        if disable_warnings:
            requests.packages.urllib3.disable_warnings()

    def build_url(self, url_path, query):
        full_url = '{}{}'.format(self.base_url, url_path)
        cgi_path = dict_to_cgi_str(query)
        if cgi_path:
            sep = '&' if '?' in full_url else '?'
            full_url = '{}{}{}'.format(full_url, sep, cgi_path)
        return full_url

    def build_headers(self, headers, is_json_data=False):
        full_headers = headers.copy()
        if self.oauth_token is not None:
            full_headers.update({'Authorization': 'OAuth {}'.format(self.oauth_token)})
        if is_json_data:
            full_headers.update({
                'Content-type': 'application/json',
                'Accept': 'application/json',
            })
        return full_headers

    def build_data(self, data, cgi_data, json_data):
        if json_data is not None:
            return json.dumps(json_data)
        elif cgi_data is not None:
            return dict_to_cgi_str(cgi_data)
        else:
            return data

    @retrying.retry(stop_max_attempt_number=3, wait_fixed=3000)
    def _request(self, url_path, query=None, data=None, cgi_data=None, json_data=None, headers=None, method='GET',
                 not_retry=False, dump_success_response=True):
        query = query or {}
        headers = headers or {}

        method = method.upper()
        http_method = self.METHODS[method]
        url = self.build_url(url_path, query)
        headers = self.build_headers(headers, json_data is not None)
        data = self.build_data(data, cgi_data, json_data)

        http_method_kwargs = {
            'url': url,
            'headers': headers,
            'verify': False,
        }

        if method in ('POST', 'PUT') and data is not None:
            http_method_kwargs['data'] = data

        response = http_method(**http_method_kwargs)

        if response.status_code != 200:
            raise ValueError('Status code {} != 200 ({} -> {})'.format(
                response.status_code, http_method_kwargs['url'][:500], response.content[:500]
            ))
        data = response.json()

        if dump_success_response:
            dump_response(url, data)
        return data

    def _get(self, url_path, query=None, headers=None, not_retry=False):
        return self._request(url_path, query=query, headers=headers, method='GET', not_retry=not_retry)

    def _post(self, url_path, query=None, data=None, cgi_data=None, json_data=None, headers=None, not_retry=False):
        return self._request(url_path, query=query, data=data, cgi_data=cgi_data, json_data=json_data, headers=headers,
                             method='POST', not_retry=not_retry)

    def _put(self, url_path, query=None, data=None, cgi_data=None, json_data=None, headers=None, not_retry=False):
        return self._request(url_path, query=query, data=data, cgi_data=cgi_data, json_data=json_data, headers=headers,
                             method='PUT', not_retry=not_retry)

    def _delete(self, url_path, query=None, headers=None, not_retry=False):
        return self._request(url_path, query=query, headers=headers, method='DELETE', not_retry=not_retry)


def dict_to_cgi_str(dict_query):
    cgi_query = ''
    for key, values in dict_query.items():
        values = values if isinstance(values, list) else [values]
        for value in values:
            sep = '&' if cgi_query else ''
            cgi_query = '{}{}{}={}'.format(cgi_query, sep, key, value)
    return cgi_query


def dump_response(url, data):
    base_dir = 'aux_dumps'
    if not os.path.exists(base_dir):
        os.mkdir(base_dir)
    filename = replace_url_chars(url)
    with open('{}/{}_{}.json'.format(base_dir, filename, int(time.time())), 'w') as wfile:
        json.dump(data, wfile, indent=4, sort_keys=True)


def replace_url_chars(url):
    for c in ('/', '?', '&', ':', ','):
        url = url.replace(c, '_')
    return url
