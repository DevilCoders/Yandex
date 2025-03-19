"""
Provider for working with kubernetes masternode
"""
import base64
import hashlib
import json
import os
import time
from functools import cached_property
from typing import Iterable, Dict
import tempfile

import yaml

from kubernetes import client, utils
from kubernetes.client import ApiException

from cloud.mdb.dbaas_worker.internal.providers.common import BaseProvider
from library.python import resource


class KubernetesMaster(BaseProvider):
    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.api_client = None
        self.namespace = None

    def set_cluster_connection_details(self, master_endpoint: str, api_key: str, cluster_certificate: str):
        configuration = client.Configuration()
        configuration.host = master_endpoint
        configuration.api_key = {'authorization': f'Bearer {api_key}'}
        configuration.ssl_ca_cert = os.path.join(
            tempfile.gettempdir(),
            f'k8s_cluster_{hashlib.sha256(master_endpoint.encode()).hexdigest()}.pem',
        )
        # TODO commit to upstream possibility to work without files
        if not os.path.exists(configuration.ssl_ca_cert):
            with open(configuration.ssl_ca_cert, 'w') as cert_file:
                cert_file.write(cluster_certificate)
                cert_file.flush()
        self.api_client = client.ApiClient(configuration)

    def set_namespace(self, namespace: str):
        self.namespace = namespace

    def _check_initialization(self):
        if not self.api_client or not self.namespace:
            raise RuntimeError('Kubernetes master connection or namespace is not initialized')

    @cached_property
    def core_api(self) -> client.CoreV1Api:
        self._check_initialization()
        return client.CoreV1Api(self.api_client)

    @cached_property
    def custom_objects_api(self) -> client.CustomObjectsApi:
        self._check_initialization()
        return client.CustomObjectsApi(self.api_client)

    @cached_property
    def batch_api(self) -> client.BatchV1Api:
        self._check_initialization()
        return client.BatchV1Api(self.api_client)

    def create_external_secret(self, resource_path: str):
        # utils.create_from_yaml will not work for custom resources such as an "external secret"
        resource_description = yaml.safe_load(resource.find(resource_path))
        group, version = resource_description['apiVersion'].split('/', 1)
        result = self.custom_objects_api.create_namespaced_custom_object(
            group=group,
            version=version,
            namespace=self.namespace,
            plural=resource_description['kind'].lower() + 's',
            body=resource_description,
        )
        return result

    def apply_configs(self, resource_paths: Iterable[str]):
        # TODO commit to upstream possibility to work without files
        with tempfile.NamedTemporaryFile() as resource_file:
            for config_path in resource_paths:
                resource_content = resource.find(config_path)
                if not resource_content:
                    raise RuntimeError(f'Can not find {config_path}')
                resource_file.write(resource_content)
                resource_file.flush()
                result = utils.create_from_yaml(
                    self.api_client,
                    resource_file.name,
                    verbose=True,
                    namespace=self.namespace,
                )
            return result

    def parse_resource(self, resource_path: str) -> dict:
        resource_content = resource.find(resource_path)
        if not resource_content:
            raise RuntimeError(f'Can not find {resource_path}')
        return yaml.safe_load(resource_content)

    def apply_config_from_dict(self, resource_dict: dict):
        result = utils.create_from_yaml(
            self.api_client,
            yaml_objects=[resource_dict],
            verbose=True,
            namespace=self.namespace,
        )
        return result

    def wait_for_job(self, job_name: str, timeout_seconds: int = 60) -> bool:
        deadline = time.time() + timeout_seconds
        while True:
            api_response = self.batch_api.read_namespaced_job_status(
                name=job_name,
                namespace=self.namespace,
            )
            if api_response:
                if api_response.status.succeeded is not None or api_response.status.failed is not None:
                    return not api_response.status.failed
            time.sleep(1)
            if time.time() > deadline:
                raise TimeoutError(f'Kubernetes job {job_name} was not finished in {timeout_seconds} seconds')

    def get_node_port(self, service_name: str) -> int:
        result = self.core_api.read_namespaced_service(namespace=self.namespace, name=service_name)
        return result.spec.ports[0].node_port

    def get_service_endpoint(self, service_name: str, timeout_seconds: int = 60) -> str:
        result = self.core_api.read_namespaced_service(namespace=self.namespace, name=service_name)
        deadline = time.time() + timeout_seconds
        while not result.status.load_balancer.ingress:
            time.sleep(1)
            result = self.core_api.read_namespaced_service(namespace=self.namespace, name=service_name)
            if time.time() > deadline:
                raise TimeoutError(
                    f'Kubernetes service {service_name} received no endpoint not finished in {timeout_seconds} seconds'
                )

        return result.status.load_balancer.ingress[0].ip

    def namespace_exists(self, name: str = None):
        body = client.V1Namespace()
        body.api_version = 'v1'
        body.kind = 'Namespace'
        body.metadata = {'name': name or self.namespace}
        try:
            return self.core_api.create_namespace(body)
        except client.exceptions.ApiException as exception:
            if json.loads(exception.body)['reason'] == 'AlreadyExists':
                pass
            else:
                raise

    def secret_exists(self, name: str, entries: Dict[str, str]):
        body = client.V1Secret()
        body.api_version = 'v1'

        data = {}
        for key, value in entries.items():
            data[key] = base64.b64encode(value.encode()).decode()

        body.data = data

        body.kind = 'Secret'
        body.metadata = {'name': name}
        body.type = 'Opaque'
        try:
            return self.core_api.create_namespaced_secret(self.namespace, body)
        except client.exceptions.ApiException as exception:
            if json.loads(exception.body)['reason'] == 'AlreadyExists':
                pass
            else:
                raise

    def configmap_exists(self, name: str, entries: Dict[str, str]):
        body = client.V1ConfigMap()
        body.api_version = 'v1'
        body.data = entries
        body.kind = 'ConfigMap'
        body.metadata = {'name': name}
        try:
            return self.core_api.create_namespaced_config_map(self.namespace, body)
        except client.exceptions.ApiException as exception:
            if json.loads(exception.body)['reason'] == 'AlreadyExists':
                pass
            else:
                raise

    def namespace_absent(self, namespace: str = None):
        namespace = namespace or self.namespace
        try:
            self.core_api.delete_namespace(name=namespace)
        except ApiException as exception:
            if exception.reason == 'NotFound':
                self.logger.info(f'namespace {namespace} was not found')
            else:
                self.logger.exception(f'error while deleting namespace {namespace}')
