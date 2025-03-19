from importlib import import_module
import logging
from typing import Any, Callable, Dict, NamedTuple

import requests
from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from .auth import get_iam_client
from .models import Cluster, TaskStatus


_REST_CLUSTERS = {
    'postgresql_cluster': 'postgresql',
    'clickhouse_cluster': 'clickhouse',
    'mongodb_cluster': 'mongodb',
    'redis_cluster': 'redis',
    'mysql_cluster': 'mysql',
    'hadoop_cluster': 'hadoop',
}


class _MDBGrpcServiceModules(NamedTuple):
    cluster_service_pb2: Any
    cluster_service_pb2_grpc: Any
    operation_service_pb2: Any
    operation_service_pb2_grpc: Any


def _make_mdb_service(name: str) -> _MDBGrpcServiceModules:
    return _MDBGrpcServiceModules(
        cluster_service_pb2=import_module(f'yandex.cloud.priv.mdb.{name}.v1.cluster_service_pb2'),
        cluster_service_pb2_grpc=import_module(f'yandex.cloud.priv.mdb.{name}.v1.cluster_service_pb2_grpc'),
        operation_service_pb2=import_module(f'yandex.cloud.priv.mdb.{name}.v1.operation_service_pb2'),
        operation_service_pb2_grpc=import_module(f'yandex.cloud.priv.mdb.{name}.v1.operation_service_pb2_grpc'),
    )


_GRPC_CLUSTERS = {
    'kafka_cluster': _make_mdb_service('kafka'),
    'elasticsearch_cluster': _make_mdb_service('elasticsearch'),
    'sqlserver_cluster': _make_mdb_service('sqlserver'),
    'greenplum_cluster': _make_mdb_service('greenplum'),
}


class _RestAPI:
    def __init__(self, config) -> None:
        self._config = config
        self._session = get_int_api_session(config)

    def delete_cluster(self, cluster: Cluster) -> str:
        response = self._session.delete(
            '{base}/mdb/{type}/v1/clusters/{cid}'.format(
                base=self._config['internal_api']['url'],
                cid=cluster.cid,
                type=_REST_CLUSTERS[cluster.type],
            )
        )
        response.raise_for_status()
        return response.json()['id']

    def stop_cluster(self, cluster: Cluster) -> str:
        response = self._session.post(
            '{base}/mdb/{type}/v1/clusters/{cid}:stop'.format(
                base=self._config['internal_api']['url'],
                cid=cluster.cid,
                type=_REST_CLUSTERS[cluster.type],
            )
        )
        response.raise_for_status()
        return response.json()['id']

    def get_task_status(self, task_id: str, cluster: Cluster) -> TaskStatus:
        response = self._session.get(
            '{base}/mdb/{type}/v1/operations/{id}'.format(
                base=self._config['internal_api']['url'], type=_REST_CLUSTERS[cluster.type], id=task_id
            )
        )
        response.raise_for_status()
        task = response.json()
        return TaskStatus(
            done=task['done'],
            failed=task.get('error', {}).get('code') is not None,
        )


class _Service(NamedTuple):
    cluster: grpcutil.WrappedGRPCService
    operations: grpcutil.WrappedGRPCService


class _GRPCApi:
    def __init__(self, config) -> None:
        self._config = config
        iam_client = get_iam_client(config)
        logger = MdbLoggerAdapter(logging.getLogger(__name__), {})
        channel = grpcutil.new_grpc_channel(
            grpcutil.Config(
                url=config['grpc_api']['url'],
                cert_file=config['grpc_api']['cert_file'],
            )
        )

        def make_service(stub) -> grpcutil.WrappedGRPCService:
            return grpcutil.WrappedGRPCService(
                logger=logger,
                channel=channel,
                grpc_service=stub,
                timeout=config['grpc_api'].get('timeout', 30.0),
                get_token=iam_client.get_token,
                error_handlers={},
            )

        self._services: Dict[str, _Service] = {}
        for cluster_type, services in _GRPC_CLUSTERS.items():
            self._services[cluster_type] = _Service(
                cluster=make_service(services.cluster_service_pb2_grpc.ClusterServiceStub),
                operations=make_service(services.operation_service_pb2_grpc.OperationServiceStub),
            )

    def delete_cluster(self, cluster: Cluster) -> str:
        request = _GRPC_CLUSTERS[cluster.type].cluster_service_pb2.DeleteClusterRequest(
            cluster_id=cluster.cid
        )
        response = self._services[cluster.type].cluster.Delete(request)
        return response.id

    def stop_cluster(self, cluster: Cluster) -> str:
        request = _GRPC_CLUSTERS[cluster.type].cluster_service_pb2.StopClusterRequest(
            cluster_id=cluster.cid
        )
        response = self._services[cluster.type].cluster.Stop(request)
        return response.id

    def get_task_status(self, task_id: str, cluster: Cluster) -> TaskStatus:
        request = _GRPC_CLUSTERS[cluster.type].operation_service_pb2.GetOperationRequest(
            operation_id=task_id
        )
        operation = self._services[cluster.type].operations.Get(request)
        return TaskStatus(
            done=operation.done,
            failed=operation.HasField('error'),
        )


class InternalAPI:
    def __init__(self, config) -> None:
        self._rest = _RestAPI(config)
        self._grpc = _GRPCApi(config)

    def _choose(self, method: str, cluster: Cluster) -> Callable:
        if cluster.type in _REST_CLUSTERS:
            return getattr(self._rest, method)
        if cluster.type in _GRPC_CLUSTERS:
            return getattr(self._grpc, method)
        raise RuntimeError(f'Unsupported cluster type: {cluster.type}')

    def delete_cluster(self, cluster: Cluster) -> str:
        return self._choose('delete_cluster', cluster)(cluster)

    def stop_cluster(self, cluster: Cluster) -> str:
        return self._choose('stop_cluster', cluster)(cluster)

    def get_task_status(self, task_id: str, cluster: Cluster) -> TaskStatus:
        return self._choose('get_task_status', cluster)(task_id, cluster)


def get_int_api_session(config):
    """
    Create requests.Session for sending requests to DBaaS Internal API.
    """
    api_config = config['internal_api']
    iam = get_iam_client(config)
    session = requests.session()
    session.headers = {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
        'X-YaCloud-SubjectToken': iam.get_token(),
    }
    session.verify = api_config['verify']
    return session
