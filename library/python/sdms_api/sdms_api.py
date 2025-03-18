#!/usr/bin/env python

from __future__ import print_function

import requests
import logging
import json


class SdmsApi(object):
    def __init__(self, srv, token=None, timeout=30, ssl_verify=True):
        self.srv = srv
        self.token = token
        self.timeout = timeout
        self.ssl_verify = ssl_verify
        self.logger = logging.getLogger(__name__)

    def _get_headers(self):
        headers = {'Content-type': 'application/json'}
        if self.token:
            headers['Authorization'] = 'OAuth ' + self.token
        return headers

    def _make_req(self, url):
        r = requests.get(self.srv + url, timeout=self.timeout, headers=self._get_headers(), verify=self.ssl_verify)
        self.logger.debug(r.url)
        if r.status_code == requests.codes.ok:
            return r.json()
        else:
            self.logger.error('return code %s: %s', r.status_code, r.text)
            raise Exception('%s: %s' % (r.status_code, r.text))

    def _make_req_put(self, url, data=None):
        r = requests.put(self.srv + url, timeout=self.timeout, data=data, headers=self._get_headers(),
                         verify=self.ssl_verify)
        self.logger.debug(r.url)
        if r.status_code == requests.codes.ok:
            return r.json()
        else:
            self.logger.error('return code %s: %s', r.status_code, r.text)
            raise Exception('%s: %s' % (r.status_code, r.text))

    def _make_req_post(self, url, data=None):
        r = requests.post(self.srv + url, timeout=self.timeout, data=data, headers=self._get_headers(),
                          verify=self.ssl_verify)
        self.logger.debug(r.url)
        if r.status_code == requests.codes.ok:
            return r.json()
        else:
            self.logger.error('return code %s: %s', r.status_code, r.text)
            raise Exception('%s: %s' % (r.status_code, r.text))

    def _make_req_del(self, url, data=None):
        r = requests.delete(self.srv + url, timeout=self.timeout, data=data, headers=self._get_headers(),
                            verify=self.ssl_verify)
        self.logger.debug(r.url)
        if r.status_code == requests.codes.ok:
            return r.json()
        elif r.status_code == requests.codes.no_content:
            return None
        else:
            self.logger.error('return code %s: %s', r.status_code, r.text)
            raise Exception('%s: %s' % (r.status_code, r.text))

    def beta_exists(self, beta_name):
        return self._make_req('/api/v2/beta/%s/exists' % beta_name)

    def get_beta_info(self, beta_name):
        return self._make_req('/api/v2/beta/%s' % beta_name)

    def get_beta_state(self, beta_name):
        return self._make_req('/api/v2/beta/%s/status' % beta_name)

    def start_beta(self, beta_name):
        return self._make_req_post('/api/v2/beta/%s/start' % beta_name)

    def stop_beta(self, beta_name):
        return self._make_req_post('/api/v2/beta/%s/stop' % beta_name)

    def delete_beta(self, beta_name):
        return self._make_req_del('/api/v2/beta/%s' % beta_name)

    def update_beta(self, beta_name, data):
        return self._make_req_post('/api/v2/beta/%s' % beta_name, json.dumps(data))

    def create_beta(self, beta_name, data):
        return self._make_req_put('/api/v2/beta/%s' % beta_name, json.dumps(data))


if __name__ == '__main__':
    logging.basicConfig(level='DEBUG')
    api = SdmsApi('https://sdms-dev.qloud.yandex-team.ru', ssl_verify=False)

    print(api.beta_exists('priemka'))
    print(api.get_beta_info('priemka'))

    data = {'config': {'priemka:WEB_BASE': {'params': {'aa', 'bb'}}}, 'start': False, 'message': 'aa', 'mode': 'UPDATE'}

    # api.update_beta('priemka', data)
