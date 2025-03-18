import requests
import json
from urlparse import urljoin
from retry import retry
from urllib3.exceptions import NewConnectionError


class InfraException(Exception):
    def __init__(self, msg, response):
        super(InfraException, self).__init__(msg)
        self.response = response


class InfraFatalException(InfraException):
    pass


class InfraRetryableException(InfraException):
    pass


class InfraClient(object):
    """
    https://infra-api.yandex-team.ru/swagger/
    """

    class EventType:
        ISSUE = 'issue'
        MAINTENANCE = 'maintenance'

    class Severity:
        MINOR = 'minor'
        MAJOR = 'major'

    def __init__(self, infra_api_url_prefix, oauth_token):
        self.oauth_token = oauth_token
        self.infra_api_url_prefix = infra_api_url_prefix

        self.__headers = {
            'Authorization': 'OAuth {}'.format(self.oauth_token),
            'Content-Type': 'application/json',
            'Accept': 'application/json'
        }

    @staticmethod
    def __raise_on_5xx(response):
        if response.status_code in (502, 503, 504):
            raise InfraRetryableException('Infra server-side error: {} {}'
                                          .format(response.status_code, response.reason), response)
        if response.status_code >= 500:
            raise InfraFatalException('Infra server-side error: {} {}'
                                      .format(response.status_code, response.reason), response)

    @retry(exceptions=(requests.ConnectTimeout, NewConnectionError, InfraRetryableException), tries=3, delay=2, backoff=2)
    def get_services(self, namespace_id=None, timeout=None):
        path = 'services' if namespace_id is None else 'namespace/{}/services'.format(namespace_id)
        response = requests.get(urljoin(self.infra_api_url_prefix, path),
                                headers=self.__headers,
                                timeout=timeout)
        self.__raise_on_5xx(response)
        if response.status_code != 200:
            raise InfraFatalException('Failed to get services (namespace_id={}): {} {}'
                                      .format(namespace_id, response.status_code, response.reason), response)
        return response.json()

    @retry(exceptions=(requests.ConnectTimeout, NewConnectionError, InfraRetryableException), tries=3, delay=2, backoff=2)
    def get_service(self, service_id, timeout=None):
        path = 'services/{}'.format(service_id)
        response = requests.get(urljoin(self.infra_api_url_prefix, path),
                                headers=self.__headers,
                                timeout=timeout)
        self.__raise_on_5xx(response)
        if response.status_code != 200:
            raise InfraFatalException('Failed to get service (service_id={}): {} {}'
                                      .format(service_id, response.status_code, response.reason), response)
        return response.json()

    @retry(exceptions=(requests.ConnectionError, NewConnectionError), tries=3, delay=2, backoff=2)
    def create_service(self, namespace_id, name):
        body = {
            'name': name,
            'namespaceId': namespace_id
        }
        response = requests.post(urljoin(self.infra_api_url_prefix, 'services'),
                                 data=json.dumps(body),
                                 headers=self.__headers)
        self.__raise_on_5xx(response)
        if response.status_code != 200:
            raise InfraFatalException('Failed to create service "{}" (namespace_id={}): {} {}'
                                      .format(name, namespace_id, response.status_code, response.reason), response)
        return response.json()

    @retry(exceptions=(requests.ConnectTimeout, NewConnectionError, InfraRetryableException), tries=3, delay=2, backoff=2)
    def get_environments(self, service_id=None, namespace_id=None, timeout=None):
        params = dict()
        if service_id is not None:
            params['serviceId'] = service_id
        if namespace_id is not None:
            params['namespaceId'] = namespace_id
        response = requests.get(urljoin(self.infra_api_url_prefix, 'environments'),
                                params=params,
                                headers=self.__headers,
                                timeout=timeout)
        self.__raise_on_5xx(response)
        if response.status_code != 200:
            raise InfraFatalException('Failed to get environments (params={}): {} {}'
                                      .format(params, response.status_code, response.reason), response)
        return response.json()

    @retry(exceptions=(requests.ConnectTimeout, NewConnectionError, InfraRetryableException), tries=3, delay=2, backoff=2)
    def get_environment(self, environment_id=None, timeout=None):
        path = 'environments/{}'.format(environment_id)
        response = requests.get(urljoin(self.infra_api_url_prefix, path),
                                headers=self.__headers,
                                timeout=timeout)
        self.__raise_on_5xx(response)
        if response.status_code != 200:
            raise InfraFatalException('Failed to get environment (env_id={}): {} {}'
                                      .format(environment_id, response.status_code, response.reason), response)
        return response.json()

    @retry(exceptions=(requests.ConnectionError, NewConnectionError), tries=3, delay=2, backoff=2)
    def create_environment(self, service_id, name, datacenters=None, create_calendar=False):
        body = {
            'serviceId': service_id,
            'name': name,
            'createCalendar': create_calendar
        }
        if datacenters is not None:
            body.update(datacenters)

        response = requests.post(urljoin(self.infra_api_url_prefix, 'environments'),
                                 data=json.dumps(body),
                                 headers=self.__headers)
        self.__raise_on_5xx(response)
        if response.status_code != 200:
            raise InfraFatalException('Failed to create environment "{}" (service_id={}): {} {}'
                                      .format(name, service_id, response.status_code, response.reason), response)
        return response.json()

    @retry(exceptions=(requests.ConnectionError, NewConnectionError), tries=3, delay=2, backoff=2)
    def modify_environment(self, environment_id, name=None, datacenters=None, calendar_id=None):
        path = 'environments/{}'.format(environment_id)
        body = dict()
        if name is not None:
            body['name'] = name
        if datacenters is not None:
            body.update(datacenters)
        if calendar_id is not None:
            body['calendarId'] = calendar_id
        response = requests.put(urljoin(self.infra_api_url_prefix, path),
                                data=json.dumps(body),
                                headers=self.__headers)
        self.__raise_on_5xx(response)
        if response.status_code != 200:
            raise InfraFatalException('Failed to modify environment (env_id={}, params={}): {} {}'
                                      .format(environment_id, body, response.status_code, response.reason), response)
        return response.json()

    @retry(exceptions=(requests.ConnectionError, NewConnectionError), tries=3, delay=2, backoff=2)
    def create_event(self,
                     service_id,
                     environment_id,
                     title,
                     desc=None,
                     meta=None,
                     start_time=None,
                     finish_time=None,
                     duration=None,
                     event_type=EventType.MAINTENANCE,
                     severity=Severity.MINOR,
                     send_email_notifications=None,  # bool
                     datacenters=None):
        """
        :param datacenters: {}, {'man': True, 'myt': True, 'iva': True}, etc.
        See Infra API reference, this interface follows the request model
        """
        body = {
            'serviceId': service_id,
            'environmentId': environment_id,
            'title': title,
            'type': event_type,
            'severity': severity
        }
        for k, v in [
            ('description', desc),
            ('meta', meta),
            ('startTime', start_time),
            ('finishTime', finish_time),
            ('duration', duration),
            ('sendEmailNotifications', send_email_notifications)
        ]:
            if v is not None:
                body[k] = v

        if datacenters is not None:
            body.update(datacenters)

        response = requests.post(urljoin(self.infra_api_url_prefix, 'events'),
                                 data=json.dumps(body),
                                 headers=self.__headers)
        self.__raise_on_5xx(response)

        if response.status_code != 200:
            raise InfraFatalException('Failed to create an event "{}" in env id={}: {} {}'
                                      .format(title, environment_id, response.status_code, response.reason), response)
        return response.json()
