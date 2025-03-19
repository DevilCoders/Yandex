import logging

from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute.clouds import CloudsClient, CloudsClientConfig
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from yandex.cloud.priv.resourcemanager.v1 import cloud_service_pb2


def get_simple_permission_stages(cloud_id, **_):
    response = cloud_service_pb2.GetPermissionStagesResponse()
    response.permission_stages.extend(['a', 'b', 'c'])
    return response


def test_clouds_client_resolve(mocker):
    retry_stop = mocker.patch('tenacity.BaseRetrying.stop')
    retry_stop.side_effect = lambda *args, **kwargs: True

    stub = mocker.patch(
        'cloud.mdb.internal.python.compute.clouds.api.CloudsClient._cloud_service',
    )
    stub.GetPermissionStages.side_effect = get_simple_permission_stages
    with CloudsClient(
        config=CloudsClientConfig(
            transport=grpcutil.Config(
                url='',
                cert_file='',
            )
        ),
        logger=MdbLoggerAdapter(logging.getLogger(__name__), {}),
        token_getter=lambda: 'dummy',
        error_handlers={},
    ) as client:
        stages = client.get_permission_stages('acloud1')
        assert stages == ['a', 'b', 'c']
