import json
import re
import requests

from tools.releaser.src.lib import https
from tools.releaser.src.conf import cfg


class QloudObject:
    """
    https://docs.qloud.yandex-team.ru/doc/api/
    """
    def __init__(self, project, application, environment, component):
        self.project = project
        self.application = application
        self.environment = environment
        self.component = component

        if project is None:
            raise ValueError('"project" must be non-empty')

        self.chain = [project]

        if application is None:
            if environment or component:
                raise ValueError('"application" is empty but "environment" or "component" is non-empty')
        else:
            self.chain.append(application)

        if environment is None:
            if component:
                raise ValueError('"environment" is empty but "component" is non-empty')
        else:
            self.chain.append(environment)

        if component is not None:
            self.chain.append(component)

    @property
    def is_project(self):
        return not self.application

    @property
    def is_application(self):
        return self.application and not self.environment

    @property
    def is_environment(self):
        return self.environment and not self.component

    @property
    def is_component(self):
        return self.component is not None

    @property
    def url(self):
        return 'projects/%s' % '/'.join(self.chain)

    def __str__(self):
        return '.'.join(self.chain)


class QloudApiException(Exception):
    pass


class QloudClient(object):
    def __init__(self, qloud_instance, oauth_token):
        """
        :type qloud_instance: str
        :type oauth_token: str
        """
        self.qloud_url = cfg.get_qloud_url(qloud_instance)
        self.session = {
            cfg.QloudInstance.INT: https.get_internal_session,
            cfg.QloudInstance.EXT: https.get_normal_session,
        }[qloud_instance]()
        self.session.headers.update({
            'Authorization': 'OAuth %s' % oauth_token
        })

    def get_project(self, project):
        """
        https://docs.qloud.yandex-team.ru/doc/api/project#getproject

        :param project: QloudObject
        :rtype: dict
        """
        if not project.is_project:
            raise ValueError('You should pass project object')

        response = self.session.get(
            '%s/api/v1/project/%s' % (self.qloud_url, str(project)),
        )
        self._check_error(response)
        return response.json()

    def get_environment_dump(self, environment):
        """
        https://docs.qloud.yandex-team.ru/doc/api/environment#getenvdump

        :type environment: QloudObject
        :rtype: dict
        """
        if not environment.is_environment:
            raise ValueError('You should pass environment object')

        response = self.session.get(
            '%s/api/v1/environment/dump/%s' % (self.qloud_url, str(environment)),
        )
        self._check_error(response)
        return response.json()

    def get_environment_status(self, environment):
        """
        Возвращает плоский список запущеных инстансов во всех компонентах окружения.
        https://docs.platform.yandex-team.ru/doc/api/information_service#getstatus

        :type environment: QloudObject
        :rtype: list
        """
        if not environment.is_environment:
            raise ValueError('You should pass environment object')

        response = self.session.get(
            '%s/api/v1/status/%s' % (self.qloud_url, str(environment)),
        )
        self._check_error(response)
        return response.json()

    def upload_environment(self, environment_dump, comment=None, target_state=None):
        """
        https://docs.qloud.yandex-team.ru/doc/api/environment#uploadenv

        :type environment_dump: dict
        :type comment: str
        """
        if comment:
            environment_dump['comment'] = comment
        else:
            del environment_dump['comment']

        params = {}
        if target_state:
            params['targetState'] = target_state

        response = self.session.post(
            '%s/api/v1/environment/upload' % self.qloud_url,
            headers={
                'Content-Type': 'application/json',
            },
            params=params,
            data=json.dumps(environment_dump),
        )
        self._check_error(response)

    def get_stable_environment(self, environment):
        """
        https://docs.qloud.yandex-team.ru/doc/api/environment#getstableenv

        :type environment: QloudObject
        :rtype: dict
        """
        if not environment.is_environment:
            raise ValueError('You should pass environment object')

        response = self.session.get(
            '%s/api/v1/environment/stable/%s' % (self.qloud_url, str(environment)),
        )
        self._check_error(response)
        return response.json()

    def delete_environment(self, environment):
        """
        https://docs.qloud.yandex-team.ru/doc/api/environment#deleteenv

        :param environment: QloudObject
        """
        if not environment.is_environment:
            raise ValueError('You should pass environment object')

        response = self.session.delete(
            '%s/api/v1/environment/stable/%s' % (self.qloud_url, str(environment)),
        )
        self._check_error(response)

    def get_domains(self, environment):
        """
        https://doc.qloud.yandex-team.ru/doc/api/domain#getdomains
        :param environment: QloudObject
        :return:
        """
        if not environment.is_environment:
            raise ValueError('You should pass environment object')
        response = self.session.get(
            '%s/api/v1/domains/%s' % (self.qloud_url, str(environment)),
            headers={
                'Content-Type': 'application/json',
            },
        )
        self._check_error(response)
        return response.json()

    def add_domain(self, environment, domain, type=None):
        """
        https://docs.qloud.yandex-team.ru/doc/api/environment#create

        :param environment: QloudObject
        :param domain: str
        """
        if not environment.is_environment:
            raise ValueError('You should pass environment object')

        data = {'domainName': domain}
        if type:
            data['type'] = type

        response = self.session.put(
            '%s/api/v1/environment-domain/%s' % (self.qloud_url, str(environment)),
            headers={
                'Content-Type': 'application/json',
            },
            data=json.dumps(data)
        )
        self._check_error(response)

    def add_deploy_hook(self, environment, deploy_hook):
        """
        https://wiki.yandex-team.ru/qloud/doc/api/deployment/#policies
        :param environment: QloudObject
        :param deploy_hook: str
        """
        if not environment.is_environment:
            raise ValueError('You should pass environment object')

        policies_url = '%s/api/v1/environment/policies/%s' % (
            self.qloud_url,
            str(environment)
        )
        response = self.session.get(
            policies_url,
            headers={
                'Content-Type': 'application/json',
            },
        )
        self._check_error(response)
        policy = response.json()

        policy['deployHook'] = deploy_hook
        response = self.session.post(policies_url, json=policy)
        self._check_error(response)

    @staticmethod
    def _check_error(response):
        try:
            response.raise_for_status()
        except requests.HTTPError as exc:
            raise QloudApiException(u"%s\n%s" % (exc, response.text))


def get_environments_by_wildcard(project_details, application, environment_wildcard):
    application_details = next((
        app for app in project_details['applications'] if app['name'] == application
    ), None)

    if not application_details:
        return None

    environments = application_details['environments']

    pattern = re.compile(environment_wildcard.replace('*', '.*'))

    return [
        QloudObject(
            project=project_details['name'],
            application=application,
            environment=environment['name'],
            component=None,
        )
        for environment in environments
        if pattern.match(environment['name'])
    ]
