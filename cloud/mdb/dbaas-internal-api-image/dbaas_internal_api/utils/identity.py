# -*- coding: utf-8 -*-
"""
Identity interaction module
"""
import logging
import time
import urllib.parse
from abc import ABC, abstractmethod
from threading import Lock, local
from typing import Any, Dict  # noqa

import requests
from flask import current_app, request

from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute.clouds import CloudsClient, CloudsClientConfig
from cloud.mdb.internal.python.compute.folders import FoldersClient, FoldersClientConfig

from dbaas_common import retry, tracing

from ..core.exceptions import CloudResolveError, FolderResolveError
from ..core.middleware import READ_ONLY_METHODS
from ..utils import iam_jwt
from ..utils.request_context import get_x_request_id
from ..utils.types import ClusterVisibility
from . import metadb
from .config import default_cloud_quota
from .logs import get_logger

PROC_CONTEXT = {}  # type: Dict[str, Any]
THREAD_CONTEXT = local()


def _get_empty_folder(folder_ext_id):
    folder = {
        'folder_ext_id': folder_ext_id,
        'folder_id': -1,
    }
    return folder


def _get_empty_cloud(cloud_ext_id):
    default_feature_flags = metadb.get_default_feature_flags()
    cloud = {
        'cloud_ext_id': cloud_ext_id,
        'cloud_id': -1,
        'memory_used': 0,
        'cpu_used': 0,
        'gpu_used': 0,
        'ssd_space_used': 0,
        'hdd_space_used': 0,
        'clusters_used': 0,
        'feature_flags': default_feature_flags or [],
    }
    cloud.update(default_cloud_quota())
    return cloud


def _create_cloud(cloud_ext_id):
    cloud = metadb.create_cloud(cloud_ext_id, default_cloud_quota())
    return cloud


def _get_cloud_by_folder_ext_id(folder_ext_id, allow_missing=False, create_missing=False):
    """
    Resolve cloud by folder_ext_id using identity api
    """
    provider = current_app.config['IDENTITY_PROVIDER'](current_app.config)
    cloud_ext_id = provider.get_cloud_by_folder_ext_id(folder_ext_id)
    cloud = metadb.get_cloud(cloud_ext_id=cloud_ext_id)
    if cloud is None:
        if allow_missing:
            if create_missing:
                return _create_cloud(cloud_ext_id)
            return _get_empty_cloud(cloud_ext_id)
        raise CloudResolveError('Unable to find cloud with cloud_ext_id = {id}'.format(id=cloud_ext_id))
    return cloud


def get_cloud_by_ext_id(*_, allow_missing=False, create_missing=False, **kwargs):
    """
    Resolve cloud by cloud_ext_id
    """
    cloud_ext_id = kwargs.get('cloud_id')
    if not cloud_ext_id:
        raise CloudResolveError('cloud_id is missing')

    cloud = metadb.get_cloud(cloud_ext_id=cloud_ext_id)
    if cloud is None:
        if allow_missing:
            if create_missing:
                ret = _create_cloud(cloud_ext_id)
                return ret, dict()
            return _get_empty_cloud(cloud_ext_id), dict()
        raise FolderResolveError('folder_id not found')
    return cloud, dict()


def get_folder_by_ext_id(*_, allow_missing=False, create_missing=False, **kwargs):
    """
    Get folder by external id.
    """
    folder_ext_id = kwargs.get('folder_id')
    if not folder_ext_id:
        raise FolderResolveError('folder_id is missing')

    folder = metadb.get_folder(folder_ext_id=folder_ext_id)
    if folder is None:
        if allow_missing:
            cloud = _get_cloud_by_folder_ext_id(
                folder_ext_id, allow_missing=allow_missing, create_missing=create_missing
            )
            if create_missing:
                ret = metadb.create_folder(folder_ext_id, cloud['cloud_id'])
                return cloud, ret
            return cloud, _get_empty_folder(folder_ext_id)
        raise FolderResolveError('Unable to find folder with folder_ext_id = {id}'.format(id=folder_ext_id))
    cloud = metadb.get_cloud(cloud_id=folder['cloud_id'])
    return cloud, folder


def get_folder_by_cluster_id(*_, visibility: ClusterVisibility = ClusterVisibility.visible, **kwargs):
    """
    Get folder by cluster name.
    """
    cluster_id = kwargs.get('cluster_id')
    if cluster_id is None:
        raise FolderResolveError('cluster_id is missing')

    folder = metadb.get_folder_by_cluster(cid=cluster_id, visibility=visibility)
    if folder is None:
        raise FolderResolveError('cluster_id not found')
    cloud = metadb.get_cloud(cloud_id=folder['cloud_id'])
    return cloud, folder


def get_folder_by_operation_id(*_, **kwargs):
    """
    Get folder by operation id.
    """
    operation_id = kwargs.get('operation_id')
    if operation_id is None:
        raise FolderResolveError('operation_id is missing')

    folder = metadb.get_folder_by_operation(operation_id=operation_id)
    if folder is None:
        raise FolderResolveError('operation_id not found')
    cloud = metadb.get_cloud(cloud_id=folder['cloud_id'])
    return cloud, folder


def merge_cloud_feature_flags(cloud):
    """
    Get cloud status by cloud_ext_id using identity api
    """
    provider = current_app.config['IDENTITY_PROVIDER'](current_app.config)
    identity_stages = provider.get_cloud_permission_stages(cloud['cloud_ext_id'])
    cloud['feature_flags'] = list(set(cloud['feature_flags']).union(set(identity_stages)))


class IdentityProvider(ABC):
    """
    Abstract Identity provider
    """

    @abstractmethod
    def get_cloud_by_folder_ext_id(self, folder_ext_id):
        """
        Resolve cloud_ext_id by folder_ext_id
        """

    @abstractmethod
    def get_cloud_permission_stages(self, cloud_ext_id):
        """
        Get permission stages (feature flags) by cloud_ext_id
        """


class YCIdentityError(Exception):
    """
    Generic YC.Identity interaction error
    """


class YCIdentityProvider(IdentityProvider):
    """
    YC.Identity-based identity provider
    """

    def __init__(self, config):
        self.token = config['YC_IDENTITY']['token']
        self.url = config['YC_IDENTITY']['base_url']
        self.ca_path = config['YC_IDENTITY']['ca_path']
        self.connect_timeout = config['YC_IDENTITY']['connect_timeout']
        self.read_timeout = config['YC_IDENTITY']['read_timeout']
        self.cache_ttl = config['YC_IDENTITY'].get('cache_ttl', 300)
        self.cache_size = config['YC_IDENTITY'].get('cache_size', 16384)
        self.logger = logging.LoggerAdapter(
            logging.getLogger(current_app.config['LOGCONFIG_BACKGROUND_LOGGER']),
            extra={
                'request_id': get_x_request_id(),
            },
        )

        self.iam_jwt = None
        if not self.token:
            self.iam_jwt = iam_jwt.get_provider()

    @retry.on_exception((YCIdentityError, requests.RequestException), max_tries=3)
    def _make_request(self, path, params=None):
        if not getattr(THREAD_CONTEXT, 'session', None):
            session = requests.Session()
            adapter = requests.adapters.HTTPAdapter(pool_connections=1, pool_maxsize=1)
            session.mount(self.url, adapter)
            THREAD_CONTEXT.session = session
        url = urllib.parse.urljoin(self.url, path)
        self.logger.debug(
            'Starting request to %s%s', url, 'with params {params}'.format(params=params) if params else ''
        )
        res = THREAD_CONTEXT.session.get(
            url,
            headers={
                'Accept': 'application/json',
                'Content-Type': 'application/json',
                'X-Request-Id': get_x_request_id(),
                'X-YaCloud-SubjectToken': self.iam_jwt.get_iam_token() if self.iam_jwt is not None else self.token,
            },
            params=params,
            verify=self.ca_path,
            timeout=(self.connect_timeout, self.read_timeout),
        )
        if res.status_code != 200:
            raise YCIdentityError(
                'Unexpected get {path} result: {status} {message}'.format(
                    path=path, status=res.status_code, message=res.text
                )
            )

        return res.json()

    @tracing.trace('IAM GetCloudByFolderExtID')
    def get_cloud_by_folder_ext_id(self, folder_ext_id):
        tracing.set_tag('cloud.folder.ext_id', folder_ext_id)

        parsed = self._make_request(f'v1/folders/{folder_ext_id}')
        cloud_id = parsed.get('cloudId')
        if not cloud_id:
            raise CloudResolveError('Unable to find folder with folder_ext_id: {id}'.format(id=folder_ext_id))

        return cloud_id

    def _gc_cache(self, ref_time):
        if (
            len(PROC_CONTEXT['permission_cache']) <= self.cache_size
            or PROC_CONTEXT['permission_cache_gc_lock'].locked()
        ):
            return
        with PROC_CONTEXT['permission_cache_gc_lock']:
            for item in sorted(PROC_CONTEXT['permission_cache'].items(), key=lambda i: i[1][0]):
                if ref_time - item[1][0] > self.cache_ttl:
                    del PROC_CONTEXT['permission_cache'][item[0]]
                elif len(PROC_CONTEXT['permission_cache']) > self.cache_size:
                    del PROC_CONTEXT['permission_cache'][item[0]]
                else:
                    break

    @tracing.trace('IAM GetPermissionStages')
    def get_cloud_permission_stages(self, cloud_ext_id):
        now = time.time()
        if 'permission_cache' not in PROC_CONTEXT:
            PROC_CONTEXT['permission_cache'] = {}
        if 'permission_cache_gc_lock' not in PROC_CONTEXT:
            PROC_CONTEXT['permission_cache_gc_lock'] = Lock()
        req_time, result = PROC_CONTEXT['permission_cache'].get(cloud_ext_id, (0, []))

        if now - req_time > self.cache_ttl:
            tracing.set_tag('cloud.ext_id', cloud_ext_id)

            cache = True

            try:
                parsed = self._make_request('v1/clouds/{cloud_id}:getPermissionStages'.format(cloud_id=cloud_ext_id))
            except Exception as exc:
                if request.method not in READ_ONLY_METHODS:
                    raise
                self.logger.error('Unable to get permission stages: %r. Assuming empty.', exc)
                parsed = {'permissionStages': []}
                cache = False

            if not parsed.get('permissionStages'):
                raise CloudResolveError('Unable to find cloud with cloud_ext_id: {id}'.format(id=cloud_ext_id))

            result = parsed['permissionStages']
            if cache:
                PROC_CONTEXT['permission_cache'][cloud_ext_id] = (now, result)
        self._gc_cache(now)
        return result


class GRPCIdentityProvider(IdentityProvider):
    """
    GRPC Identity-based identity provider
    """

    def __init__(self, config):
        self.cache_ttl = config['YC_IDENTITY'].get('cache_ttl', 300)
        self.cache_size = config['YC_IDENTITY'].get('cache_size', 16384)

        self.logger = MdbLoggerAdapter(
            get_logger(),
            extra={
                'request_id': get_x_request_id(),
            },
        )
        self.iam_jwt = iam_jwt.get_provider()

        rm_config = config['RESOURCE_MANAGER_CONFIG_GRPC']
        self._folders_client_config = FoldersClientConfig(
            transport=grpcutil.Config(
                url=rm_config['url'],
                cert_file=rm_config['cert_file'],
            )
        )
        self._cloud_client_config = CloudsClientConfig(
            transport=grpcutil.Config(
                url=rm_config['url'],
                cert_file=rm_config['cert_file'],
            )
        )

    def _gc_cache(self, ref_time):
        if (
            len(PROC_CONTEXT['permission_cache']) <= self.cache_size
            or PROC_CONTEXT['permission_cache_gc_lock'].locked()
        ):
            return
        with PROC_CONTEXT['permission_cache_gc_lock']:
            for item in sorted(PROC_CONTEXT['permission_cache'].items(), key=lambda i: i[1][0]):
                if ref_time - item[1][0] > self.cache_ttl:
                    del PROC_CONTEXT['permission_cache'][item[0]]
                elif len(PROC_CONTEXT['permission_cache']) > self.cache_size:
                    del PROC_CONTEXT['permission_cache'][item[0]]
                else:
                    break

    @tracing.trace('IAM Folders Resolve')
    def get_cloud_by_folder_ext_id(self, folder_ext_id):
        tracing.set_tag('cloud.folder.ext_id', folder_ext_id)
        with FoldersClient(
            config=self._folders_client_config,
            logger=self.logger,
            token_getter=self.iam_jwt.get_iam_token,
            error_handlers={},
        ) as client:
            cloud_ids = client.resolve([folder_ext_id])
            if len(cloud_ids) != 1:
                raise CloudResolveError('Unable to find folder with folder_ext_id: {id}'.format(id=folder_ext_id))
            return cloud_ids[0].cloud_id

    @tracing.trace('IAM Clouds GetPermissionStages')
    def get_cloud_permission_stages(self, cloud_ext_id):
        now = time.time()
        if 'permission_cache' not in PROC_CONTEXT:
            PROC_CONTEXT['permission_cache'] = {}
        if 'permission_cache_gc_lock' not in PROC_CONTEXT:
            PROC_CONTEXT['permission_cache_gc_lock'] = Lock()
        req_time, result = PROC_CONTEXT['permission_cache'].get(cloud_ext_id, (0, []))

        if now - req_time > self.cache_ttl:
            tracing.set_tag('cloud.ext_id', cloud_ext_id)

            cache = True

            try:
                with CloudsClient(
                    config=self._cloud_client_config,
                    logger=self.logger,
                    token_getter=self.iam_jwt.get_iam_token,
                    error_handlers={},
                ) as client:
                    result = client.get_permission_stages(cloud_ext_id)
            except Exception as exc:
                self.logger.error('Unable to get permission stages: %r. Assuming empty.', exc)
                result = []
                cache = False

            if cache:
                PROC_CONTEXT['permission_cache'][cloud_ext_id] = (now, result)
        self._gc_cache(now)
        return result
