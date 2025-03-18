import logging
import os
import six

if six.PY2:
    from urlparse import urljoin
elif six.PY3:
    from urllib.parse import urljoin

from uuid import uuid4

import requests
from requests.packages.urllib3.util import Retry

_log = logging.getLogger('step_client')


DEFAULT_HOSTS = {
    'testing': 'https://step-sandbox1.n.yandex-team.ru',
    'production': 'https://step.sandbox.yandex-team.ru',
}

ENV_HOSTS = {
    'development': 'testing',
    'prestable': 'testing',
}


def _byteify(data):
    if isinstance(data, six.string_types):
        return str(data)

    if isinstance(data, list):
        return [_byteify(item) for item in data]

    if isinstance(data, dict):
        return {
            _byteify(key): _byteify(value) for key, value in six.iteritems(data)
        }

    return data


class Client(requests.Session):

    def __init__(self, env=None, host=None, token=None, timeout=60, verify=False, num_retries=5):
        super(Client, self).__init__()

        if not token:
            raise Exception("Set oauth token")

        self.headers = {'Authorization': 'OAuth %s' % token}
        self.verify = verify

        if env and host:
            raise Exception("Specify either env or host")
        elif host:
            self._host = host
        elif env:
            host_env = ENV_HOSTS.get(env, env)
            self._host = DEFAULT_HOSTS[host_env]
        else:
            raise Exception("Specify either env or host")

        self._timeout = timeout

        for prefix in ['http://', 'https://']:
            self.mount(
                prefix,
                requests.adapters.HTTPAdapter(
                    max_retries=Retry(
                        total=num_retries,
                        backoff_factor=0.5,
                        status_forcelist=[500, 502, 503, 504, 521],
                        method_whitelist=frozenset(
                            ['HEAD', 'TRACE', 'GET', 'PUT', 'OPTIONS', 'DELETE', 'POST'])
                    )
                )
            )

    def _url(self, rel_url):
        return urljoin(self._host, rel_url)

    def request(self, method, url, **kwargs):
        resp = super(Client, self).request(
            method, url, timeout=self._timeout, **kwargs
        )
        if 400 <= resp.status_code < 500:
            resp.reason = resp.content
        resp.raise_for_status()
        return resp

    def create_events(self, events_list, sandbox_task_id=None):
        request_id = str(uuid4())

        post_data = {
            'request_id': request_id,
            'events': events_list,
        }
        if sandbox_task_id is not None:
            post_data['source_type'] = 'SANDBOX_TASK'
            post_data['source'] = {'task_id': sandbox_task_id}

        elif 'STATBOX_RUN_ID' in os.environ:
            try:
                post_data['source_type'] = 'SANDBOX_TASK'
                post_data['source'] = {
                    'task_id': os.environ['STATBOX_RUN_ID'],
                    'primary_task_id': os.environ['STATBOX_TASK_ID'],
                    'statinfra_action_id': os.environ['STATBOX_ACTION_ID'],
                }
            except Exception:
                _log.exception('Failed to attach statinfra info')

        resp = self.post(
            self._url('api/v1/events'),
            json=post_data
        )
        return resp.json(object_hook=_byteify)['ids']

    def create_event(self, name, params, sandbox_task_id=None):
        return self.create_events([dict(name=name, params=params)], sandbox_task_id=sandbox_task_id)[0]

    def get_events(self, name=None, params=None, skip=0, limit=30,
                   state=None, sort=None):
        params = params or {}
        req_params = {'params__%s' % k: v for k, v in list(params.items())}

        if name is not None:
            req_params['name'] = name

        req_params['skip'] = skip
        req_params['limit'] = limit

        if state is not None:
            req_params['state'] = state

        if sort is not None:
            req_params['sort'] = sort

        resp = self.get(
            self._url('api/v1/events'),
            params=req_params
        )
        return resp.json(object_hook=_byteify)['result']

    def get_event(self, event_id):
        resp = self.get(
            self._url('api/v1/events/%s' % event_id)
        )
        return resp.json(object_hook=_byteify)['result']

    def reject_event(self, event_id):
        resp = self.post(
            self._url('api/v1/reject_event'),
            json={
                'id': event_id
            }
        )
        return resp.json(object_hook=_byteify)['id']

    def request_event_story(self, event_id):
        resp = self.post(
            self._url('api/v1/event_story'),
            json={
                'id': event_id
            }
        )
        return resp.json(object_hook=_byteify)['id']

    def get_event_story(self, story_id):
        resp = self.get(
            self._url('api/v1/event_story/%s' % story_id)
        )
        return resp.json(object_hook=_byteify)['result']

    def get_event_description(self, event_name):
        resp = self.get(
            self._url('api/v1/event_desc/%s' % event_name)
        )
        return resp.json(object_hook=_byteify)['result']

    def create_event_description(self, event_name, required_fields, optional_fields,
                                 description, list_in_idm=False, public=True):
        self.post(
            self._url('api/v1/event_desc'),
            json={
                'name': event_name,
                'required_fields': required_fields,
                'optional_fields': optional_fields,
                'description': description,
                'list_in_idm': list_in_idm,
                'public': public,
            }
        )

    def update_event_description(self, event_name, **kwargs):
        possible_keys = ('required_fields', 'optional_fields',
                         'description', 'list_in_idm', 'public')

        invalid_keys = sorted(set(kwargs) - set(possible_keys))
        if invalid_keys:
            raise Exception('Invalid keys: %s' % invalid_keys)

        self.put(
            self._url('api/v1/event_desc/%s' % event_name),
            json=kwargs
        )

    def delete_event_description(self, event_name):
        self.delete(
            self._url('api/v1/event_desc/%s' % event_name)
        )

    def create_config(
            self,
            event_deps,
            action_type='SANDBOX_TASK',
            semaphores=None,
            **action_params
    ):
        action_params.update(dict(
            event_deps=event_deps,
            action_type=action_type,
            semaphores=semaphores or [],
        ))
        resp = self.post(
            self._url('api/v1/configs'),
            json=action_params
        )
        return resp.json(object_hook=_byteify)['id']

    def update_config(self, config_id, data):
        self.put(
            self._url('api/v1/configs/%s' % config_id),
            json=data
        )

    def delete_config(self, config_id):
        self.delete(
            self._url('api/v1/configs/%s' % config_id),
        )

    def get_config(self, config_id):
        return self.get(
            self._url('api/v1/configs/%s' % config_id),
        ).json(object_hook=_byteify)['result']

    def get_configs(self, **kwargs):
        return self.get(
            self._url('api/v1/configs'),
            params=kwargs,
        ).json(object_hook=_byteify)['result']
