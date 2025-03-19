"""
Implements low-level API calls

NOTE: TM API is subject to change.
No attempt was made to make this basic requests wrapper look like a full-blown API client.
"""
import json
import logging
import time

import requests


LOG = logging.getLogger('tm-api')


def api_response(fun):
    def wrapper(*args, **kwargs):
        try:
            data = fun(*args, **kwargs)
            if not data.ok:
                LOG.debug(f'API response: {data.text}')
            data.raise_for_status()
            return data.json()
        except requests.exceptions.RequestException as exc:
            LOG.exception(f'unable to call TM: {exc} with body: {data.text}')
            raise
    return wrapper


class TM:

    def __init__(self, endpoint, folder_id, token, ca_file=None, iam_url=None):
        self._base_url = f'{endpoint}/v1'
        self._folder_id = folder_id
        self._ca_file = ca_file or '/etc/ssl/certs/ca-certificates.crt'
        self._headers = self._make_headers(token, iam_url)

    def _make_headers(self, token, iam_url=None):
        auth_header = f'oAuth {token}'
        if token.startswith('t1.'):
            auth_header = f'Bearer {token}'
        return {
            'Authorization': auth_header,
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        }

    @api_response
    def list_transfers(self, status='TRANSFER_STATUS_UNSPECIFIED', folder_id=None):
        if folder_id is None:
            folder_id = self._folder_id
        url = f'{self._base_url}/transfers/list/{folder_id}?status={status}'
        return requests.get(url, headers=self._headers, verify=self._ca_file)

    @api_response
    def list_endpoints(self, folder_id=None):
        if folder_id is None:
            folder_id = self._folder_id
        url = f'{self._base_url}/endpoints/list/{folder_id}'
        return requests.get(url, headers=self._headers, verify=self._ca_file)

    @api_response
    def get_transfer(self, transfer_id):
        url = f'{self._base_url}/transfer/{transfer_id}'
        return requests.get(url, headers=self._headers, verify=self._ca_file)

    @api_response
    def create_transfer(self, connector_spec):
        """ Creates a Logfeller-parsed stream directed into Clickhouse """
        url = f'{self._base_url}/connector/lf/ch'
        LOG.debug('create lf/ch transfer request: %s', connector_spec)
        return requests.post(
            url, json=connector_spec, headers=self._headers, verify=self._ca_file)

    @api_response
    def create_transfer_by_id(self, transfer_spec):
        """ Creates a general-purpose transfer using two ids """
        url = f'{self._base_url}/transfer'
        LOG.debug('create transfer request: %s', transfer_spec)
        return requests.post(
            url, json=transfer_spec, headers=self._headers, verify=self._ca_file)


    @api_response
    def create_endpoint(self, spec):
        url = f'{self._base_url}/endpoint'
        LOG.debug('create endpoint request: %s', spec)
        return requests.post(
            url, json=spec, headers=self._headers, verify=self._ca_file)

    @api_response
    def start(self, connector_id):
        url = f'{self._base_url}/transfer/activate'
        return requests.patch(
            url,
            json={'transfer_id': connector_id},
            headers=self._headers,
            verify=self._ca_file)

    @api_response
    def delete_endpoint(self, endpoint_id):
        url = f'{self._base_url}/endpoint/{endpoint_id}'
        return requests.delete(
            url,
            headers=self._headers,
            verify=self._ca_file)

    @api_response
    def delete(self, transfer_id):
        url = f'{self._base_url}/transfer/{transfer_id}'
        return requests.delete(
            url,
            headers=self._headers,
            verify=self._ca_file)

    @api_response
    def deactivate(self, transfer_id):
        url = f'{self._base_url}/transfer/{transfer_id}:deactivate'
        return requests.post(
            url,
            json={'transfer_id': transfer_id},
            headers=self._headers,
            verify=self._ca_file)

    @api_response
    def list_logs(self, entity_id):
        url = f'{self._base_url}/transfers/logs?log_group_id={entity_id}'
        return requests.get(url, headers=self._headers, verify=self._ca_file)

    def transfer_wait_state(self, transfer_id, state, timeout):
        begin = time.time()
        while time.time() - begin < timeout:
            transfer = self.get_transfer(transfer_id)
            cur_state = transfer['status']
            if cur_state == state:
                return True
            LOG.debug(
                'waiting for %s to become %s: currently %s',
                transfer_id,
                state,
                cur_state,
            )
            time.sleep(1)
        raise RuntimeError(f'Timed out waiting for {transfer_id} to become {state}')


def getter(method, field, **kwargs):
    try:
        return method(**kwargs)[field]
    except KeyError:
        return []

