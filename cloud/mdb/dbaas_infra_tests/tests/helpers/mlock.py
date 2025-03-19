"""
Mlock api utils
"""
import grpc
from grpc_health.v1.health_pb2 import HealthCheckRequest, HealthCheckResponse
from grpc_health.v1.health_pb2_grpc import HealthStub

from cloud.mdb.mlock.api import lock_service_pb2, lock_service_pb2_grpc
from tests.helpers import docker


def get_channel(context):
    """
    Get gRPC connection to Mlock api.
    """
    host, port = docker.get_exposed_port(
        docker.get_container(context, 'mlock01'), context.conf['projects']['mlock']['expose']['grpc'])
    return grpc.insecure_channel(f'{host}:{port}')


def get_metadata(context):
    """
    Get gRPC authorization metadata.
    """
    return [('authorization', f'Bearer {context.conf["mlock"]["token"]}')]


def is_ready(context):
    """
    Check if mlock service is ready to serve requests
    """
    with get_channel(context) as channel:
        response = HealthStub(channel).Check(HealthCheckRequest())
        if response.status == HealthCheckResponse.SERVING:  # pylint: disable=no-member
            return True
    return False


def lock_cluster(context, hosts):
    """
    Lock cluster hosts
    """
    with get_channel(context) as channel:
        stub = lock_service_pb2_grpc.LockServiceStub(channel)
        request = lock_service_pb2.CreateLockRequest()
        request.id = 'infra-test-lock'
        request.reason = 'infra test'
        request.holder = 'test'
        request.objects.extend(hosts)  # pylint: disable=no-member
        stub.CreateLock(request, metadata=get_metadata(context))


def unlock_cluster(context):
    """
    Unlock cluster hosts
    """
    with get_channel(context) as channel:
        stub = lock_service_pb2_grpc.LockServiceStub(channel)
        request = lock_service_pb2.ReleaseLockRequest()
        request.id = 'infra-test-lock'
        stub.ReleaseLock(request, metadata=get_metadata(context))
