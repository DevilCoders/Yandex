# -*- coding: utf-8 -*-
"""
Authentication & authorization module.
"""

import logging
from abc import ABC, abstractmethod
from functools import wraps
from threading import local
from typing import Optional

from dbaas_common import retry, tracing
from dbaas_common.tracing import grpc_channel_tracing_interceptor
from flask import current_app, g, request
from flask_restful import abort
from grpc import RpcError, channel_ready_future, insecure_channel, secure_channel, ssl_channel_credentials
from psycopg2 import Error
from psycopg2.pool import PoolError
from yc_as_client import YCAccessServiceClient
from yc_as_client.exceptions import (
    CancelledException,
    InternalException,
    TimeoutException,
    UnauthenticatedException,
    UnavailableException,
)
from yc_auth.authentication import TokenAuth
from yc_auth.authorization import authorize
from yc_auth.exceptions import ConnectionError as YcAuthConnectionError
from yc_auth.exceptions import InternalServerError, YcAuthClientError

from ..core.exceptions import CloudResolveError, FolderResolveError, UnsupportedAuthActionError
from ..utils.identity import get_cloud_by_ext_id, get_folder_by_ext_id, merge_cloud_feature_flags
from ..utils.register import Resource, DbaasOperation, get_auth_action
from ..utils.request_context import get_x_request_id

AUTH_CONTEXT = local()

DEFAULT_AUTH_ACTIONS = {
    Resource.ALERT_GROUP: {
        DbaasOperation.CREATE: 'mdb.all.create',
        DbaasOperation.DELETE: 'mdb.all.delete',
        DbaasOperation.INFO: 'mdb.all.read',
        DbaasOperation.LIST: 'mdb.all.read',
        DbaasOperation.MODIFY: 'mdb.all.modify',
    },
    Resource.BACKUP: {
        DbaasOperation.CREATE: 'mdb.all.modify',
        DbaasOperation.INFO: 'mdb.all.read',
        DbaasOperation.LIST: 'mdb.all.read',
        DbaasOperation.RESTORE_HINTS: 'mdb.all.read',
    },
    Resource.CLUSTER: {
        DbaasOperation.ADD_ZOOKEEPER: 'mdb.all.create',
        DbaasOperation.BILLING_CREATE: 'mdb.all.read',
        DbaasOperation.CREATE: 'mdb.all.create',
        DbaasOperation.CREATE_DICTIONARY: 'mdb.all.modify',
        DbaasOperation.DELETE: 'mdb.all.delete',
        DbaasOperation.DELETE_DICTIONARY: 'mdb.all.modify',
        DbaasOperation.ENABLE_SHARDING: 'mdb.all.modify',
        DbaasOperation.INFO: 'mdb.all.read',
        DbaasOperation.LIST: 'mdb.all.read',
        DbaasOperation.MODIFY: 'mdb.all.modify',
        DbaasOperation.MOVE: 'mdb.all.modify',
        DbaasOperation.REBALANCE: 'mdb.all.modify',
        DbaasOperation.RESTORE: 'mdb.all.create',
        DbaasOperation.START: 'mdb.all.modify',
        DbaasOperation.START_FAILOVER: 'mdb.all.modify',
        DbaasOperation.STOP: 'mdb.all.modify',
    },
    Resource.CONSOLE_CLUSTERS_CONFIG: {
        DbaasOperation.INFO: 'mdb.all.read',
    },
    Resource.DATABASE: {
        DbaasOperation.CREATE: 'mdb.all.create',
        DbaasOperation.DELETE: 'mdb.all.delete',
        DbaasOperation.INFO: 'mdb.all.read',
        DbaasOperation.LIST: 'mdb.all.read',
        DbaasOperation.MODIFY: 'mdb.all.modify',
    },
    Resource.FORMAT_SCHEMA: {
        DbaasOperation.CREATE: 'mdb.all.create',
        DbaasOperation.DELETE: 'mdb.all.delete',
        DbaasOperation.INFO: 'mdb.all.read',
        DbaasOperation.LIST: 'mdb.all.read',
        DbaasOperation.MODIFY: 'mdb.all.modify',
    },
    Resource.HADOOP_JOB: {
        DbaasOperation.CANCEL: 'mdb.all.modify',
        DbaasOperation.CREATE: 'mdb.all.create',
        DbaasOperation.GET_HADOOP_JOB_LOG: 'mdb.all.read',
        DbaasOperation.INFO: 'mdb.all.read',
        DbaasOperation.LIST: 'mdb.all.read',
    },
    Resource.HADOOP_UI_LINK: {
        DbaasOperation.LIST: 'mdb.all.read',
    },
    Resource.HOST: {
        DbaasOperation.BATCH_CREATE: 'mdb.all.create',
        DbaasOperation.BATCH_DELETE: 'mdb.all.delete',
        DbaasOperation.BATCH_MODIFY: 'mdb.all.modify',
        DbaasOperation.BILLING_CREATE_HOSTS: 'mdb.all.modify',
        DbaasOperation.LIST: 'mdb.all.read',
    },
    Resource.MAINTENANCE: {
        DbaasOperation.RESCHEDULE: 'mdb.all.modify',
    },
    Resource.ML_MODEL: {
        DbaasOperation.CREATE: 'mdb.all.create',
        DbaasOperation.DELETE: 'mdb.all.delete',
        DbaasOperation.INFO: 'mdb.all.read',
        DbaasOperation.LIST: 'mdb.all.read',
        DbaasOperation.MODIFY: 'mdb.all.modify',
    },
    Resource.OPERATION: {
        DbaasOperation.INFO: 'mdb.all.read',
        DbaasOperation.LIST: 'mdb.all.read',
    },
    Resource.SHARD: {
        DbaasOperation.CREATE: 'mdb.all.create',
        DbaasOperation.DELETE: 'mdb.all.delete',
        DbaasOperation.INFO: 'mdb.all.read',
        DbaasOperation.LIST: 'mdb.all.read',
        DbaasOperation.MODIFY: 'mdb.all.modify',
    },
    Resource.SUBCLUSTER: {
        DbaasOperation.CREATE: 'mdb.all.create',
        DbaasOperation.DELETE: 'mdb.all.delete',
        DbaasOperation.INFO: 'mdb.all.read',
        DbaasOperation.LIST: 'mdb.all.read',
        DbaasOperation.MODIFY: 'mdb.all.modify',
    },
    Resource.USER: {
        DbaasOperation.CREATE: 'mdb.all.create',
        DbaasOperation.DELETE: 'mdb.all.delete',
        DbaasOperation.GRANT_PERMISSION: 'mdb.all.modify',
        DbaasOperation.INFO: 'mdb.all.read',
        DbaasOperation.LIST: 'mdb.all.read',
        DbaasOperation.MODIFY: 'mdb.all.modify',
        DbaasOperation.REVOKE_PERMISSION: 'mdb.all.modify',
    },
}


class AuthError(Exception):
    """
    Generic Auth error
    Auth provider should rise such errors on non-retriable cases
    """


class AuthProvider(ABC):
    """
    Abstract Auth Provider
    """

    @abstractmethod
    def authenticate(self, request_obj):
        """
        Check auth and return auth context
        """

    @abstractmethod
    def authorize(self, action, token, folder_id=None, cloud_id=None, sa_id=None):
        """
        Check that user with token is allowed
        to do action in cloud/folder
        """

    @abstractmethod
    def ping(self):
        """
        Check auth and return auth context
        """


class AccessServiceAuthProvider(AuthProvider):
    """
    YC.AccessService-based auth provider
    """

    def __init__(self, config):
        self.host, port = config['YC_ACCESS_SERVICE']['endpoint'].split(':')
        self.port = int(port)
        self.config = config
        self.timeout = self.config['YC_ACCESS_SERVICE']['timeout']
        self.auth_method = TokenAuth(None, self._get_access_service_client)

    def _get_access_service_client(self):
        if not getattr(AUTH_CONTEXT, 'access_service_client', None):
            conf = self.config['YC_ACCESS_SERVICE']
            options = (
                ('grpc.keepalive_time_ms', conf.get('GRPC_KEEPALIVE_TIME_MS', 11000)),
                ('grpc.keepalive_timeout_ms', conf.get('GRPC_KEEPALIVE_TIMEOUT_MS', 1000)),
                ('grpc.keepalive_permit_without_calls', conf.get('GRPC_KEEPALIVE_PERMIT_WITHOUT_CALLS', True)),
                ('grpc.http2.max_pings_without_data', conf.get('GRPC_MAX_PINGS_WITHOUT_DATA', 0)),
                (
                    'grpc.http2.min_ping_interval_without_data_ms',
                    conf.get('GRPC_MIN_PING_INTERVAL_WITHOUT_DATA_MS', 5000),
                ),
            )
            if 'ca_path' in conf:
                with open(conf['ca_path'], 'rb') as cert:
                    channel = secure_channel(
                        conf['endpoint'],
                        ssl_channel_credentials(root_certificates=cert.read()),
                        options=options,
                    )
            else:
                channel = insecure_channel(conf['endpoint'], options=options)
            AUTH_CONTEXT.channel = grpc_channel_tracing_interceptor(channel)
            AUTH_CONTEXT.access_service_client = YCAccessServiceClient(
                channel=AUTH_CONTEXT.channel, timeout=self.timeout
            )

        return AUTH_CONTEXT.access_service_client

    def ping(self):
        self._get_access_service_client()
        channel_ready_future(AUTH_CONTEXT.channel).result(timeout=self.timeout)

    @tracing.trace('AccessService authenticate')
    def authenticate(self, request_obj):
        try:
            return self.auth_method.authenticate_flask_request(request_obj)
        except YcAuthClientError as exc:
            raise AuthError(*exc.args)

    @tracing.trace('AccessService authorize')
    def authorize(self, action, token, folder_id=None, cloud_id=None, sa_id=None):
        try:
            authorize(
                None,
                self._get_access_service_client(),
                action,
                token,
                folder_id=folder_id,
                cloud_id=(None if folder_id else cloud_id),
                sa_id=(None if folder_id or cloud_id else sa_id),
                request_id=get_x_request_id(),
            )
        except (UnauthenticatedException, YcAuthClientError) as exc:
            raise AuthError(*exc.args)


def check_action(action='mdb.all.modify', folder_ext_id=None, cloud_ext_id=None):
    """
    Check if action is allowed for folder or cloud
    """
    cache_key = f'{action}_{folder_ext_id}_{cloud_ext_id}'
    if request.auth_context['actions_cache'].get(cache_key):
        return
    request.auth_context['provider'].authorize(action, request.auth_context['token'], folder_ext_id, cloud_ext_id)
    request.auth_context['actions_cache'][cache_key] = True
    _save_success_authorization(
        action,
        entity_type='folder' if folder_ext_id else 'cloud',
        folder_ext_id=folder_ext_id,
        cloud_ext_id=cloud_ext_id,
    )


def _save_success_authorization(action, entity_type, folder_ext_id, cloud_ext_id):
    if 'authorizations' not in request.auth_context:
        request.auth_context['authorizations'] = []

    request.auth_context['authorizations'].append(
        {
            'action': action,
            'entity_type': entity_type,
            'entity_id': folder_ext_id if entity_type == 'folder' else cloud_ext_id,
        }
    )


def check_auth(
    explicit_action=None,
    entity_type='folder',
    folder_resolver=get_folder_by_ext_id,
    cloud_resolver=get_cloud_by_ext_id,
    resource: Optional[Resource] = None,
    operation: Optional[DbaasOperation] = None,
):
    """
    Auth decorator
    """

    def wrapper(callback):
        """
        Wrapper function (returns internal wrapper)
        """

        @wraps(callback)
        @retry.on_exception(
            (
                CancelledException,
                TimeoutException,
                UnavailableException,
                InternalException,
                RpcError,
                YcAuthConnectionError,
                InternalServerError,
                PoolError,
                Error,
            ),
            factor=10,
            max_wait=2,
            max_tries=3,
        )
        def auth_check_wrapper(*args, **kwargs):
            """
            Get folder/cloud from resolver and check auth
            """
            logger = logging.LoggerAdapter(
                logging.getLogger(current_app.config['LOGCONFIG_BACKGROUND_LOGGER']),
                extra={
                    'request_id': get_x_request_id(),
                },
            )

            if explicit_action is not None:
                action = explicit_action
            else:
                if "cluster_type" not in kwargs:
                    raise UnsupportedAuthActionError(
                        'Improperly configured handler: cluster_type not found in kwargs '
                        'and "explicit_action" was not passed'
                    )

                action = get_auth_action(
                    cluster_type=kwargs["cluster_type"],
                    resource=resource,
                    operation=operation,
                )

            try:
                if getattr(g, 'metadb_context', None):
                    g.metadb.close_context()
                create_missing = current_app.config['IDENTITY']['create_missing'] and action == 'mdb.all.create'
                allow_missing = current_app.config['IDENTITY']['create_missing']

                provider = current_app.config['AUTH_PROVIDER'](current_app.config)

                logger.info('Starting auth with %s', provider)
                context = provider.authenticate(request)

                g.user_id = context.user.id
                request.auth_context = {
                    'user_id': context.user.id,
                    'token': context.token,
                    'provider': provider,
                    'actions_cache': {},
                    'authentication': {
                        'user_type': context.user.type,
                    },
                }

                if entity_type == 'folder':
                    cloud, folder = folder_resolver(
                        *args, **kwargs, allow_missing=allow_missing, create_missing=create_missing
                    )
                else:
                    cloud, folder = cloud_resolver(
                        *args, **kwargs, allow_missing=allow_missing, create_missing=create_missing
                    )
                override_folder = current_app.config['IDENTITY']['override_folder']
                override_cloud = current_app.config['IDENTITY']['override_cloud']
                cloud_id = override_cloud if override_cloud else cloud.get('cloud_ext_id')
                folder_id = override_folder if override_folder else folder.get('folder_ext_id')
                logger.info(
                    'Checking permissions with action ' '%s, folder %s and cloud %s', action, folder_id, cloud_id
                )
                provider.authorize(action, context.token, folder_id, cloud_id)

                merge_cloud_feature_flags(cloud)
                g.cloud = cloud
                g.folder = folder
                request.auth_context.update(
                    {
                        'folder_id': folder.get('folder_id'),
                        'folder_ext_id': folder.get('folder_ext_id'),
                        'cloud_id': cloud['cloud_id'],
                        'cloud_ext_id': cloud['cloud_ext_id'],
                        'feature_flags': ','.join(sorted(cloud['feature_flags'])),
                    }
                )
                _save_success_authorization(
                    action,
                    entity_type,
                    folder_ext_id=g.folder.get('folder_ext_id'),
                    cloud_ext_id=g.cloud['cloud_ext_id'],
                )

                return callback(*args, **kwargs)
            except (CloudResolveError, FolderResolveError, AuthError) as exc:
                logger.info('Rejecting due to {exc!r}'.format(exc=exc))
                abort(
                    403, message='You do not have permission to access the ' 'requested object or object does not exist'
                )

        return auth_check_wrapper

    return wrapper
