import logging
import grpc
from concurrent import futures

import yatest.common.network as network

import cloud.blockstore.public.sdk.python.protos as protos
import cloud.blockstore.public.api.grpc.service_pb2_grpc as service_pb2_grpc

from cloud.blockstore.public.sdk.python.client.discovery import CreateDiscoveryClient


class NbsServiceMock(service_pb2_grpc.TBlockStoreServiceServicer):

    def __init__(self, endpoint):
        super(NbsServiceMock, self).__init__()
        self.__endpoint = endpoint

    def Ping(self, request, context):
        return protos.TPingResponse()

    def DiscoverInstances(self, request, context):
        response = protos.TDiscoverInstancesResponse()

        n = request.Limit if request.Limit > 0 else 10

        response.Instances.add(
            Host=self.__endpoint.split(':')[0],
            Port=int(self.__endpoint.split(':')[1]),
        )

        for i in range(n-1):
            response.Instances.add(
                Host=f'myt1-ct5-{i + 1}.cloud.yandex.net',
                Port=9766
            )

        return response

    def QueryAvailableStorage(self, request, context):
        response = protos.TQueryAvailableStorageResponse()

        for i, name in enumerate(request.AgentIds):
            response.AvailableStorage.add(
                AgentId=name,
                ChunkSize=93*1024**3,
                ChunkCount=i*3
            )

        return response


def test_discovery():
    logging.basicConfig()

    pm = network.PortManager()

    endpoint = f'localhost:{pm.get_port()}'

    server = grpc.server(futures.ThreadPoolExecutor(max_workers=1))

    service_pb2_grpc.add_TBlockStoreServiceServicer_to_server(
        NbsServiceMock(endpoint),
        server
    )

    server.add_insecure_port(endpoint)
    server.start()

    client = CreateDiscoveryClient([endpoint])

    response = client.discover_instances(limit=3)

    assert len(response.Instances) == 3

    assert response.Instances[0].Host == 'localhost'
    assert response.Instances[0].Port == int(endpoint.split(':')[1])

    for i, ep in enumerate(response.Instances[1:]):
        assert ep.Host == f'myt1-ct5-{i + 1}.cloud.yandex.net'
        assert ep.Port == 9766

    server.stop(grace=10)
    server.wait_for_termination()

    client.close()


def test_query_available_storage():
    logging.basicConfig()

    pm = network.PortManager()

    endpoint = f'localhost:{pm.get_port()}'

    server = grpc.server(futures.ThreadPoolExecutor(max_workers=1))

    service_pb2_grpc.add_TBlockStoreServiceServicer_to_server(
        NbsServiceMock(endpoint),
        server
    )

    server.add_insecure_port(endpoint)
    server.start()

    client = CreateDiscoveryClient([endpoint])

    storage = client.query_available_storage([
        f'myt1-ct5-{i + 1}.cloud.yandex.net' for i in range(3)
    ])

    assert len(storage) == 3
    for i, info in enumerate(storage):
        assert info.AgentId == f'myt1-ct5-{i + 1}.cloud.yandex.net'
        assert info.ChunkSize == 93*1024**3
        assert info.ChunkCount == i*3

    server.stop(grace=10)
    server.wait_for_termination()

    client.close()
