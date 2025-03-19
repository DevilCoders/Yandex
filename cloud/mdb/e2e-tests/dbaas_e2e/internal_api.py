"""
DBaaS E2E tests module with internal API client
"""
import logging
import time
from abc import ABC, abstractmethod
from typing import Iterable
from urllib.parse import urljoin
from uuid import uuid4

import grpc
import requests
from dbaas_common import retry as dbaas_retry
from google.protobuf.json_format import MessageToDict, ParseDict
from retrying import retry
from yandex.cloud.priv.mdb.v1 import operation_service_pb2 as op_spec
from yandex.cloud.priv.mdb.v1.operation_service_pb2_grpc import OperationServiceStub
from datacloud.clickhouse.v1 import operation_service_pb2 as dc_op_clickhouse_spec
from datacloud.clickhouse.v1.operation_service_pb2_grpc import (
    OperationServiceStub as OperationServiceDoubleCloudClickhouseStub,
)
from datacloud.kafka.v1 import operation_service_pb2 as dc_op_kafka_spec
from datacloud.kafka.v1.operation_service_pb2_grpc import OperationServiceStub as OperationServiceDoubleCloudKafkaStub

from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute.iam import jwt
from cloud.mdb.internal.python.logs import MdbLoggerAdapter


class ApiError(RuntimeError):
    """
    Generic Internal API client error
    """


class ApiRequestError(ApiError):
    """
    Internal API error with bad HTTP status code
    """

    def __init__(self, status, method, uri, error):
        super().__init__()
        self.status = status
        self.method = method
        self.uri = uri
        self.error = error

    def __str__(self):
        return '{code} {method} {uri}: {error}'.format(
            code=self.status,
            method=self.method,
            uri=self.uri,
            error=self.error,
        )


class Operation:
    """Represents API operation metadata."""

    def __init__(self, operation_id, cluster_id, done=False, error=None):
        self.id = operation_id
        self.cluster_id = cluster_id
        self.done = done
        self.error = error


class Cluster:
    """Represents cluster metadata."""

    def __init__(self, cluster_id, cluster_name):
        self.id = cluster_id
        self.name = cluster_name


class OperationError(ApiError):
    """
    Internal API operation-related error
    """

    def __init__(self, operation: Operation):
        super().__init__()
        self.operation = operation

    def __str__(self):
        return 'Operation \'{op}\' has error \'{error}\''.format(
            op=self.operation.id,
            error=self.operation.error,
        )


class ApiClient(ABC):
    """
    ApiClient Interface
    """

    CLIENTS = dict()

    def __init__(self, config, logger):
        self._folder = config.folder_id
        self._network_id = config.network_id
        self._ca = config.ca_path
        self._timeout = config.timeout
        self._interval = config.interval
        self.logger = logger

    @classmethod
    def from_type(cls, typ, config, logger, *args, **kwargs):
        """
        Factory to create needed API client which was registered
        via `self.subclass` decorator
        """
        if typ not in cls.CLIENTS:
            raise ApiError('Unknown client type {typ}'.format(typ=typ))
        return cls.CLIENTS[typ](config, logger, *args, **kwargs)

    @classmethod
    def subclass(cls, typ):
        """
        Decorator to register subclass in `self.CLIENTS`
        """

        # pylint: disable=missing-docstring
        def wrapper(typ_cls):
            cls.CLIENTS[typ] = typ_cls
            return typ_cls

        return wrapper

    def cleanup_folder(self, cluster_type_filter, cluster_name, wait=True):
        """
        Cleanup all clusters in folder
        """
        self.logger.info('Cleanup all clusters in folder')
        clusters = self.list_clusters(cluster_type_filter)
        self.logger.info('Found %s clusters', len(clusters))
        for cluster in clusters:
            if cluster.name == cluster_name:
                self.delete_cluster(cluster_type_filter, cluster, wait=wait)

    def wait_operation(self, operation_id, raise_on_failure=True, timeout=None):
        done = None
        timeout = time.time() + (timeout or self._timeout)
        operation = None
        error = None
        while not done and time.time() < timeout and (not time.sleep(self._interval)):
            operation = self.operation_info(operation_id)
            done = operation.done
            error = operation.error
            self.logger.info(
                'Checking done for operation %s (cluster %s): %s', operation.id, operation.cluster_id, done
            )
        if error and raise_on_failure:
            raise OperationError(operation)
        if time.time() > timeout:
            raise ApiError('Operation {op} timed out after {timeout} seconds'.format(op=operation.id, timeout=timeout))

    # pylint: disable=too-many-arguments
    def create_cluster(self, cluster_type, name, env, options, wait=False) -> Operation:
        """
        Create cluster and wait it if need
        """
        self.logger.info('Creating new \'%s\' cluster', cluster_type)
        idempotence_id = str(uuid4())
        operation = self._create_cluster(cluster_type, name, env, options, idempotence_id)
        if wait:
            self.wait_operation(operation.id)
        return operation

    # pylint: disable=too-many-arguments
    @abstractmethod
    def _create_cluster(self, cluster_type, name, env, options, idempotence_id) -> Operation:
        """
        Create cluster with given params
        """

    @abstractmethod
    def cluster_hosts(self, cluster_type, cluster_id):
        """
        Show cluster info
        """

    @abstractmethod
    def get_cluster(self, cluster_id):
        """
        Get cluster info
        """

    def delete_cluster(self, cluster_type, cluster: Cluster, wait=False):
        """
        Delete cluster by cluster_id and wait it if need
        """
        self.logger.info('Deleting cluster %s', cluster.id)
        idempotence_id = str(uuid4())
        operation = self._delete_cluster(cluster_type, cluster, idempotence_id)
        if wait:
            self.wait_operation(operation.id)

    @abstractmethod
    def _delete_cluster(self, cluster_type, cluster: Cluster, idempotence_id) -> Operation:
        """
        Delete cluster
        """

    @abstractmethod
    def list_clusters(self, cluster_type_filter) -> Iterable[Cluster]:
        """
        List all clusters in folder
        """

    @abstractmethod
    def operation_info(self, operation_id) -> Operation:
        """
        Returns operation object
        """


retry_api = retry(wait_exponential_multiplier=1000, wait_exponential_max=10000, stop_max_attempt_number=3)


@ApiClient.subclass('internal')
class InternalApiClient(ApiClient):
    """
    Internal API client
    """

    def __init__(self, config, logger, version='v1'):
        super().__init__(config, logger)
        self._request_timeout = config.request_timeout
        self._url = urljoin(config.api_url, 'mdb/')
        self._version = version

        sa_creds = jwt.SACreds(
            service_account_id=config.sa_creds['service_account_id'],
            key_id=config.sa_creds['id'],
            private_key=config.sa_creds['private_key'],
        )

        jwt_config = jwt.Config(
            transport=grpcutil.Config(
                url=config.iam['host'],
                cert_file=config.iam['cert_file'],
            ),
            audience='https://iam.api.cloud.yandex.net/iam/v1/tokens',
            request_expire=3600,
            expire_thresh=180,
        )

        self._iam_jwt = jwt.IamJwt(
            config=jwt_config,
            logger=logging.LoggerAdapter(self.logger, {}),
            sa_creds=sa_creds,
        )

    # pylint: disable=no-self-use
    def _raise_on_bad_status(self, response, uri, method):
        if int(response.status_code) >= 300:
            raise ApiRequestError(response.status_code, method, uri, response.text)

    def _token(self) -> str:
        return self._iam_jwt.get_token()

    @retry_api
    def _make_request(self, uri, method='GET', data=None, headers=None, **kwargs):
        req_headers = {'X-YaCloud-SubjectToken': self._token()}
        if headers:
            req_headers.update(headers)
        response = requests.request(
            method.lower(),
            urljoin(self._url, uri),
            verify=self._ca,
            json=data,
            timeout=self._request_timeout,
            headers=req_headers,
            **kwargs
        )
        self._raise_on_bad_status(response, uri, method)
        return response.json()

    def list_clusters(self, cluster_type_filter) -> Iterable[Cluster]:
        """
        List all clusters in folder
        """
        prefix = '{cluster_type}/{version}'.format(cluster_type=cluster_type_filter, version=self._version)
        response = self._make_request(
            '{prefix}/clusters'.format(prefix=prefix),
            params={
                'folderId': self._folder,
            },
        )
        return [Cluster(c['id'], c['name']) for c in response['clusters']]

    def _delete_cluster(self, cluster_type, cluster, idempotence_id) -> Operation:
        """
        Delete cluster by cluster_id
        """
        data = self._make_request(
            '{cluster_type}/{version}/clusters/{cluster_id}'.format(
                cluster_type=cluster_type, cluster_id=cluster.id, version=self._version
            ),
            method='DELETE',
            headers={'Idempotency-Key': idempotence_id},
        )
        return self.json_to_operation(data)

    # pylint: disable=too-many-arguments
    def _create_cluster(self, cluster_type, name, env, options, idempotence_id) -> Operation:
        """
        Create cluster
        """
        data = self._make_request(
            '{cluster_type}/{version}/clusters'.format(cluster_type=cluster_type, version=self._version),
            method='POST',
            data={
                'name': name,
                'environment': env,
                'folderId': self._folder,
                'networkId': self._network_id,
                **options,
            },
            headers={'Idempotency-Key': idempotence_id},
        )
        return self.json_to_operation(data)

    def cluster_hosts(self, cluster_type, cluster_id):
        """
        Returns cluster hosts
        """
        return self._make_request(
            '{cluster_type}/{version}/clusters/{cluster_id}/hosts'.format(
                cluster_type=cluster_type,
                cluster_id=cluster_id,
                version=self._version,
            ),
            params={
                'folderId': self._folder,
            },
        )

    def get_cluster(self, cluster_id):
        """
        Get cluster info
        """
        return None

    def operation_info(self, operation_id) -> Operation:
        """
        Returns operation object
        """
        data = self._make_request('{version}/operations/{op_id}'.format(version=self._version, op_id=operation_id))
        return self.json_to_operation(data)

    def dataproc_job_create(self, cluster_id, job=None, wait=True):
        """
        Create dataproc job for running cluster
        """
        data = {'cluster_id': cluster_id}
        if job is not None:
            data.update(job)
        operation = self._make_request(
            'hadoop/{version}/clusters/{cluster_id}/jobs'.format(version=self._version, cluster_id=cluster_id),
            method='POST',
            data=data,
        )
        if wait:
            self.wait_operation(operation['id'], raise_on_failure=False)
        return operation

    def dataproc_job_info(self, cluster_id, job_id):
        """
        Retrieve dataproc job
        """
        response = self._make_request(
            'hadoop/{version}/clusters/{cluster_id}/jobs/{job_id}'.format(
                version=self._version, cluster_id=cluster_id, job_id=job_id
            )
        )
        return response

    @staticmethod
    def json_to_operation(data):
        return Operation(data['id'], data['metadata']['clusterId'], data['done'], data.get('error', None))


client_retry = dbaas_retry.on_exception(
    (
        grpcutil.exceptions.DeadlineExceededError,
        grpcutil.exceptions.AuthenticationError,
        grpcutil.exceptions.TemporaryUnavailableError,
    ),
    factor=10,
    max_wait=60,
    max_tries=6,
)


@ApiClient.subclass('internal-grpc')
class InternalGrpcApiClient(InternalApiClient):
    """
    Internal Grpc API client
    """

    def __init__(self, config, logger, grpc_defs):
        super().__init__(config, logger, version='v1')
        with open(config.ca_path, 'rb') as f:
            creds = grpc.ssl_channel_credentials(f.read())

        self._grpc_defs = grpc_defs
        self._grpc_channel = grpc.secure_channel(config.grpc_api_url, creds)
        logger = MdbLoggerAdapter(self.logger, extra={})
        self._cluster_service = grpcutil.WrappedGRPCService(
            logger,
            self._grpc_channel,
            self._grpc_defs['cluster_service']['stub'],
            timeout=self._request_timeout,
            get_token=self._token,
            error_handlers={},
        )
        self._operation_service = grpcutil.WrappedGRPCService(
            logger,
            self._grpc_channel,
            OperationServiceStub,
            timeout=self._request_timeout,
            get_token=self._token,
            error_handlers={},
        )

    @client_retry
    def list_clusters(self, cluster_type_filter) -> Iterable[Cluster]:
        req = self._grpc_defs['cluster_service']['requests']['list_clusters']
        msg = self._to_pb(req, {'folder_id': self._folder, 'page_size': 100})  # TODO: paging
        resp = self._cluster_service.List(msg)
        return [Cluster(c['id'], c['name']) for c in self._from_pb(resp).get('clusters', [])]

    @staticmethod
    def _to_pb(msg_type, values):
        msg = msg_type()
        ParseDict(values, msg)
        return msg

    @staticmethod
    def _from_pb(msg):
        return MessageToDict(msg, including_default_value_fields=True, preserving_proto_field_name=True)

    @client_retry
    def _create_cluster(self, cluster_type, name, env, options, idempotence_id) -> Operation:
        req = self._grpc_defs['cluster_service']['requests']['create']
        opts = {**options, 'name': name, 'environment': env}
        msg = self._to_pb(req, opts)
        resp = self._cluster_service.Create(msg, idempotency_key=idempotence_id)
        return self._pb_to_operation(resp)

    @client_retry
    def _delete_cluster(self, cluster_type, cluster: Cluster, idempotence_id) -> Operation:
        req = self._grpc_defs['cluster_service']['requests']['delete']
        msg = self._to_pb(req, {'cluster_id': cluster.id})
        resp = self._cluster_service.Delete(msg, idempotency_key=idempotence_id)
        return self._pb_to_operation(resp)

    @client_retry
    def operation_info(self, operation_id) -> Operation:
        msg = self._to_pb(op_spec.GetOperationRequest, {'operation_id': operation_id})
        response = self._operation_service.Get(msg)
        return self._pb_to_operation(response)

    @client_retry
    def cluster_hosts(self, cluster_type, cluster_id, list_method='ListHosts'):
        req = self._grpc_defs['cluster_service']['requests']['list_hosts']
        msg = self._to_pb(req, {'cluster_id': cluster_id, 'page_size': 100})
        if cluster_type == 'greenplum' and list_method == 'ListHosts':
            list_method = 'ListMasterHosts'
        host_list_func = getattr(self._cluster_service, list_method)
        resp = host_list_func(msg)
        return self._from_pb(resp)

    def _pb_to_operation(self, pb_resp) -> Operation:
        data = self._from_pb(pb_resp)
        return Operation(data['id'], data['metadata']['cluster_id'], data['done'], data.get('error', None))


class DoubleCloudInternalGrpcApiClient(InternalGrpcApiClient):
    """
    Internal Grpc API client for DoubleCloud
    """

    def __init__(self, config, logger, grpc_defs, operation_service_spec, operation_service_stub):
        super().__init__(config, logger, grpc_defs)
        self._project_id = config.project_id
        self.operation_service_spec = operation_service_spec

        logger = MdbLoggerAdapter(self.logger, extra={})
        self._operation_service = grpcutil.WrappedGRPCService(
            logger,
            self._grpc_channel,
            operation_service_stub,
            timeout=self._request_timeout,
            get_token=self._token,
            error_handlers={},
        )

    @client_retry
    def list_clusters(self, cluster_type_filter) -> Iterable[Cluster]:
        req = self._grpc_defs['cluster_service']['requests']['list_clusters']
        msg = self._to_pb(req, {'project_id': self._project_id, 'paging': {'page_size': 100}})
        resp = self._cluster_service.List(msg)
        return [Cluster(c['id'], c['name']) for c in self._from_pb(resp).get('clusters', [])]

    @client_retry
    def _create_cluster(self, cluster_type, name, env, options, idempotence_id) -> Operation:
        req = self._grpc_defs['cluster_service']['requests']['create']
        opts = {**options, 'name': name}
        msg = self._to_pb(req, opts)
        resp = self._cluster_service.Create(msg, idempotency_key=idempotence_id)
        return self._pb_to_operation(resp)

    @client_retry
    def _delete_cluster(self, cluster_type, cluster: Cluster, idempotence_id) -> Operation:
        req = self._grpc_defs['cluster_service']['requests']['delete']
        msg = self._to_pb(req, {'cluster_id': cluster.id})
        resp = self._cluster_service.Delete(msg, idempotency_key=idempotence_id)
        return self._pb_to_operation(resp)

    def _operation_info_pb_to_operation(self, pb_resp) -> Operation:
        data = self._from_pb(pb_resp)
        return Operation(
            data['id'], data['metadata']['cluster_id'], data['status'] == 'STATUS_DONE', data.get('error', None)
        )

    @client_retry
    def operation_info(self, operation_id) -> Operation:
        msg = self._to_pb(self.operation_service_spec.GetOperationRequest, {'operation_id': operation_id})
        response = self._operation_service.Get(msg)
        return self._operation_info_pb_to_operation(response)

    def _pb_to_operation(self, pb_resp) -> Operation:
        data = self._from_pb(pb_resp)
        operation_id = data['operation_id']
        msg = self._to_pb(self.operation_service_spec.GetOperationRequest, {'operation_id': operation_id})
        op_response = self._operation_service.Get(msg)
        operation_data = self._from_pb(op_response)
        return Operation(
            data['operation_id'],
            operation_data['metadata']['cluster_id'],
            operation_data['status'] == 'STATUS_DONE',
            operation_data.get('error', None),
        )

    def cluster_hosts(self, cluster_type, cluster_id, list_method='ListHosts'):
        req = self._grpc_defs['cluster_service']['requests']['list_hosts']
        msg = self._to_pb(req, {'cluster_id': cluster_id, 'paging': {'page_size': 100}})
        idempotence_id = str(uuid4())
        resp = self._cluster_service.ListHosts(msg, idempotency_key=idempotence_id)
        return self._from_pb(resp)

    def get_cluster(self, cluster_id):
        req = self._grpc_defs['cluster_service']['requests']['get']
        msg = self._to_pb(req, {'cluster_id': cluster_id, 'sensitive': True})
        idempotence_id = str(uuid4())
        resp = self._cluster_service.Get(msg, idempotency_key=idempotence_id)
        return self._from_pb(resp)


@ApiClient.subclass('doublecloud-internal-clickhouse-grpc')
class DoubleCloudInternalClickhouseGrpcApiClient(DoubleCloudInternalGrpcApiClient):
    def __init__(self, config, logger, grpc_defs):
        super().__init__(config, logger, grpc_defs, dc_op_clickhouse_spec, OperationServiceDoubleCloudClickhouseStub)


@ApiClient.subclass('doublecloud-internal-kafka-grpc')
class DoubleCloudInternalKafkaGrpcApiClient(DoubleCloudInternalGrpcApiClient):
    def __init__(self, config, logger, grpc_defs):
        super().__init__(config, logger, grpc_defs, dc_op_kafka_spec, OperationServiceDoubleCloudKafkaStub)
