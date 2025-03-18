from contextlib import contextmanager

from antiadblock.libs.infra.lib.infra_client import InfraException
from antiadblock.configs_api.lib.db_utils import create_lock
from antiadblock.configs_api.lib.db import db


class InfraCacheManager(object):
    def __init__(self, infra_client, namespace_id):
        self.infra_client = infra_client
        self.namespace_id = namespace_id

        self.__infra_service_cache = dict()
        self.__infra_service_envs_cache = dict()

    @contextmanager
    def __sync(self):
        try:
            create_lock('INFRA_MANAGER')
            yield
        finally:
            db.session.commit()

    def invalidate_cache(self):
        self.__infra_service_cache = dict()
        self.__infra_service_envs_cache = dict()

    def __recache_services(self, retrieval_timeout=None):
        self.__infra_service_cache = dict()
        infra_services = self.infra_client.get_services(namespace_id=self.namespace_id, timeout=retrieval_timeout)
        for service in infra_services:  # service ~ {'id': 123, 'name': 'service name'}
            self.__infra_service_cache[service['name']] = service

    def __recache_environments_for_service(self, service_name, retrieval_timeout=None):
        if service_name not in self.__infra_service_cache.keys():
            raise LookupError('there is no service "{}" in cache')

        service = self.__infra_service_cache[service_name]
        self.__infra_service_envs_cache[service_name] = dict()

        service_envs = self.infra_client.get_environments(service_id=service['id'],
                                                          namespace_id=self.namespace_id,
                                                          timeout=retrieval_timeout)
        envs_cache = dict()
        for env in service_envs:
            # env ~ {'id': ..., 'name': ..., 'service_id': ..., datacenters..., 'namespace_id': ...}
            envs_cache[env['name']] = env
        self.__infra_service_envs_cache[service_name] = envs_cache

    def __check_service_exists(self, service_name, retrieval_timeout=None):
        service = self.__infra_service_cache.get(service_name)
        if service is None:
            return False
        try:
            self.infra_client.get_service(service['id'], timeout=retrieval_timeout)
        except InfraException as e:
            if e.response.status_code == 404:
                return False
            raise e
        return True

    def get_or_create_infra_service(self, service_name, retrieval_timeout=None):
        if self.__check_service_exists(service_name, retrieval_timeout=retrieval_timeout):
            return self.__infra_service_cache[service_name]

        with self.__sync():
            # double check
            self.__recache_services(retrieval_timeout=retrieval_timeout)
            if service_name in self.__infra_service_cache.keys():
                return self.__infra_service_cache[service_name]

            # create service
            infra_service = self.infra_client.create_service(self.namespace_id, service_name)

        if 'id' not in infra_service.keys():
            if 'service_id' not in infra_service.keys():
                raise AssertionError("neither 'id' nor 'service_id' is specified in response for created service")
            infra_service['id'] = infra_service['service_id']
        self.__infra_service_cache[service_name] = infra_service
        return infra_service

    def __check_environment_exists(self, service_name, environment_name, retrieval_timeout=None):
        env = self.__infra_service_envs_cache.get(service_name, dict()).get(environment_name)
        if env is None:
            return False
        try:
            self.infra_client.get_environment(env['id'], timeout=retrieval_timeout)
        except InfraException as e:
            if e.response.status_code == 404:
                return False
            raise e
        return True

    def get_or_create_infra_environment(self, service_name, environment_name, retrieval_timeout=None):
        if self.__check_service_exists(service_name, retrieval_timeout=retrieval_timeout):
            if self.__check_environment_exists(service_name, environment_name, retrieval_timeout=retrieval_timeout):
                return self.__infra_service_envs_cache[service_name][environment_name]

        service = self.get_or_create_infra_service(service_name, retrieval_timeout=retrieval_timeout)

        with self.__sync():
            # double check
            self.__recache_environments_for_service(service_name, retrieval_timeout=retrieval_timeout)
            env = self.__infra_service_envs_cache.get(service_name, dict()).get(environment_name)
            if env is not None:
                return env

            # IMPORTANT: if an environment is deleted from Infra,
            # we won't be able to recreate an environment with the same name
            # (but Infra admins can restore the deleted environment)

            # create env
            infra_env = self.infra_client.create_environment(service_id=service['id'], name=environment_name)

        if service_name not in self.__infra_service_envs_cache.keys():
            self.__infra_service_envs_cache[service_name] = dict()
        self.__infra_service_envs_cache[service_name][environment_name] = infra_env
        return infra_env
