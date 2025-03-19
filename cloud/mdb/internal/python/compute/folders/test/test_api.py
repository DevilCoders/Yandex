import logging

from functools import partial

from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute.folders import FoldersClient, FoldersClientConfig
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from yandex.cloud.priv.resourcemanager.v1 import folder_service_pb2
from yandex.cloud.priv.access import access_pb2


def resolve_equals(request, **_):
    response = folder_service_pb2.ResolveFoldersResponse()
    for folder_id in request.folder_ids:
        resolved_folder = response.resolved_folders.add()
        resolved_folder.id = folder_id
        resolved_folder.cloud_id = folder_id
    return response


def test_folders_client_resolve(mocker):
    retry_stop = mocker.patch('tenacity.BaseRetrying.stop')
    retry_stop.side_effect = lambda *args, **kwargs: True

    stub = mocker.patch(
        'cloud.mdb.internal.python.compute.folders.api.FoldersClient._folder_service',
    )
    stub.Resolve.side_effect = resolve_equals
    with FoldersClient(
        config=FoldersClientConfig(
            transport=grpcutil.Config(
                url='',
                cert_file='',
            )
        ),
        logger=MdbLoggerAdapter(logging.getLogger(__name__), {}),
        token_getter=lambda: 'dummy',
        error_handlers={},
    ) as client:
        folders = client.resolve(['abc'])
        assert len(folders) == 1
        assert folders[0].cloud_id == 'abc'


def list_page_with_n_bindings(n, request, **_):
    response = access_pb2.ListAccessBindingsResponse()
    page_begin = 0 if not request.page_token else int(request.page_token)
    n_remained = n - page_begin
    page_end = page_begin
    if n_remained <= request.page_size:
        page_end += n_remained
    else:
        page_end += request.page_size
        response.next_page_token = str(page_end)

    for i in range(page_begin, page_end):
        binding = response.access_bindings.add()
        binding.role_id = f'role{i}'
        binding.subject.id = f'subject{i}'
        binding.subject.type = f'type{i}'
    return response


def test_folders_client_list_access_bindings_single_page(mocker):
    retry_stop = mocker.patch('tenacity.BaseRetrying.stop')
    retry_stop.side_effect = lambda *args, **kwargs: True

    stub = mocker.patch(
        'cloud.mdb.internal.python.compute.folders.api.FoldersClient._folder_service',
    )
    stub.ListAccessBindings.side_effect = partial(list_page_with_n_bindings, 1)
    with FoldersClient(
        config=FoldersClientConfig(
            transport=grpcutil.Config(
                url='',
                cert_file='',
            )
        ),
        logger=MdbLoggerAdapter(logging.getLogger(__name__), {}),
        token_getter=lambda: 'dummy',
        error_handlers={},
    ) as client:
        bindings = client.list_access_bindings('folder1')
        bindings = list(bindings)
        assert len(bindings) == 1
        assert bindings[0].role_id == 'role0'
        assert bindings[0].subject.id == 'subject0'
        assert bindings[0].subject.type == 'type0'


def test_folders_client_list_access_bindings_several_pages(mocker):
    retry_stop = mocker.patch('tenacity.BaseRetrying.stop')
    retry_stop.side_effect = lambda *args, **kwargs: True

    stub = mocker.patch(
        'cloud.mdb.internal.python.compute.folders.api.FoldersClient._folder_service',
    )
    stub.ListAccessBindings.side_effect = partial(list_page_with_n_bindings, 5)
    with FoldersClient(
        config=FoldersClientConfig(
            transport=grpcutil.Config(
                url='',
                cert_file='',
            ),
            page_size=2,
        ),
        logger=MdbLoggerAdapter(logging.getLogger(__name__), {}),
        token_getter=lambda: 'dummy',
        error_handlers={},
    ) as client:
        bindings = client.list_access_bindings('folder1')
        bindings = list(bindings)
        assert len(bindings) == 5
        for i in range(5):
            assert bindings[i].role_id == f'role{i}'
