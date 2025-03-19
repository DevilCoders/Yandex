# -*- coding: utf-8 -*-
"""
DBaaS Internal API backups management
"""

from functools import partial
from itertools import islice
from typing import Iterable, List, Optional, TypeVar

from flask import current_app, g
from flask.views import MethodView

from . import API, marshal, parse_kwargs
from ..core.auth import check_action, check_auth
from ..core.exceptions import (
    BackupNotExistsError,
    DbaasClientError,
    DbaasNotImplementedError,
    FolderResolveError,
    UnsupportedHandlerError,
)
from ..utils import metadb, pagination
from ..utils.backup_id import MalformedGlobalBackupId, encode_global_backup_id
from ..utils.cluster.get import get_all_clusters_info_in_folder, get_cluster_info_assert_exists, locked_cluster_info
from ..utils.idempotence import supports_idempotence
from ..utils.identity import get_folder_by_cluster_id, get_folder_by_ext_id
from ..utils.logs import log_warn
from ..utils.register import DbaasOperation, Resource, get_request_handler
from ..utils.types import Backup, ClusterInfo, ClusterStatus, ClusterVisibility
from ..utils.validation import check_cluster_not_in_status
from ..utils.backups import get_cluster_id
from .operations import render_operation_v1
from .schemas.backups import ListBackupsRequestSchemaV1, ListClusterBackupsRequestSchemaV1
from .schemas.operations import OperationSchemaV1


class BackupProxy:
    """
    Proxy backup attributes
    """

    __slots__ = ('back', 'folder_id', 'cluster_id')

    def __init__(self, back: Backup, folder_id: str, cluster_id: str) -> None:
        self.back = back
        self.folder_id = folder_id
        self.cluster_id = cluster_id

    @property
    def id(self):  # pylint: disable=invalid-name
        """
        Return global backup id
        """
        return encode_global_backup_id(self.cluster_id, self.back.id)

    def __getattr__(self, name):
        return getattr(self.back, name)

    @staticmethod
    def sort_key(bpr):
        """
        Sort key for backup
        """
        # can't simply sort by end_time,
        # cause this sort not stable
        return (bpr.back.end_time, bpr.cluster_id)


BT = TypeVar('BT', Backup, BackupProxy)


def get_backups_page(backups: Iterable[BT], limit: int, page_token_id: Optional[str]) -> List[BT]:
    """
    Return limit elements after page_token_id
    """
    backups_iter = iter(backups)
    if page_token_id is not None:
        for bak in backups_iter:
            if bak.id == page_token_id:
                break
    return list(islice(backups_iter, limit))


backups_paging = partial(
    pagination.supports_pagination,
    items_field='backups',
    columns=[pagination.AttributeColumn(field='id', field_type=str)],
)


def get_cluster_backups(info: ClusterInfo) -> List[BackupProxy]:
    """
    Get cluster backups and wrap then in BackupProxies
    """
    try:
        request_handler = get_request_handler(info.type, Resource.BACKUP, DbaasOperation.LIST)
    except UnsupportedHandlerError:
        raise DbaasNotImplementedError('Backups listing for {0} not implemented'.format(info.type))
    backups = request_handler(info)

    folder_id = g.folder['folder_ext_id']
    return [BackupProxy(back, folder_id, info.cid) for back in backups]


@API.resource('/mdb/<ctype:cluster_type>/<version>/clusters/<string:cluster_id>/backups')
class ClusterListBackupsV1(MethodView):
    """
    List cluster backups
    """

    @parse_kwargs.with_schema(ListClusterBackupsRequestSchemaV1)
    @marshal.with_resource(Resource.BACKUP, DbaasOperation.LIST)
    @check_auth(
        folder_resolver=get_folder_by_cluster_id,
        resource=Resource.BACKUP,
        operation=DbaasOperation.LIST,
    )
    @backups_paging()
    def get(self, cluster_type: str, cluster_id: str, limit: int, page_token_id: str = None, **_) -> List[BackupProxy]:
        """
        Get cluster backups
        """
        with metadb.commit_on_success():
            info = get_cluster_info_assert_exists(cluster_id, cluster_type)
            backups = get_cluster_backups(info)
            backups.sort(key=BackupProxy.sort_key, reverse=True)
            return get_backups_page(backups=backups, page_token_id=page_token_id, limit=limit)


@API.resource('/mdb/<ctype:cluster_type>/<version>/backups')
class ListBackupsV1(MethodView):
    """
    List folder backups
    """

    @parse_kwargs.with_schema(ListBackupsRequestSchemaV1)
    @marshal.with_resource(Resource.BACKUP, DbaasOperation.LIST)
    @check_auth(resource=Resource.BACKUP, operation=DbaasOperation.LIST)
    @backups_paging()
    def get(
        self,
        cluster_type: str,
        folder_id: str,  # pylint: disable=unused-argument
        limit: int,
        page_token_id: str = None,
        **_,
    ) -> List[BackupProxy]:
        """
        Get folder backups
        """
        backups = []  # type: List[BackupProxy]
        for info in get_all_clusters_info_in_folder(cluster_type, limit=None, include_deleted=True):
            backups += get_cluster_backups(info)
        backups.sort(key=BackupProxy.sort_key, reverse=True)

        return get_backups_page(backups=backups, page_token_id=page_token_id, limit=limit)


def get_folder_by_backup_id(*args, **kwargs):
    """
    Get folder by backup_id
    """
    backup_id = kwargs.get('backup_id')
    if backup_id is None:
        raise FolderResolveError('backup_id missing')
    try:
        cluster_id = get_cluster_id(backup_id)
    except MalformedGlobalBackupId as exc:
        log_warn('Got Malformed backup_id: %s', exc)
        raise FolderResolveError('Malformed backup_id')

    backup_cloud, backup_folder = get_folder_by_cluster_id(
        cluster_id=cluster_id,
        visibility=ClusterVisibility.visible_or_deleted,
    )
    check_action(action='mdb.all.read', folder_ext_id=backup_folder['folder_ext_id'])
    if kwargs.get('folder_id'):
        cloud, folder = get_folder_by_ext_id(*args, **kwargs)
        if (
            cloud['cloud_ext_id'] != backup_cloud['cloud_ext_id']
            and not current_app.config['IDENTITY']['allow_move_between_clouds']
        ):
            raise DbaasClientError('Unable to create cluster in non-original cloud')
        return cloud, folder
    return backup_cloud, backup_folder


def get_cluster_backup_by_id(info: ClusterInfo, backup_id: str) -> BackupProxy:
    """
    Get cluster backup by backup_id

    raise NotExists if this backup not found
    """
    for back in get_cluster_backups(info):
        if back.id == backup_id:
            return back
    raise BackupNotExistsError(backup_id)


@API.resource('/mdb/<ctype:cluster_type>/<version>/backups/<string:backup_id>')
class GetBackupV1(MethodView):
    """
    Get backup info
    """

    # don't use any schema,
    # cause we have only backup_id
    @marshal.with_resource(Resource.BACKUP, DbaasOperation.INFO)
    @check_auth(
        folder_resolver=get_folder_by_backup_id,
        resource=Resource.BACKUP,
        operation=DbaasOperation.INFO,
    )
    def get(self, cluster_type: str, backup_id: str, **_) -> BackupProxy:
        """
        Get backup info by its id
        """
        # don't catch MalformedGlobalBackupId,
        # cause we check_auth by cid extracted from backup_id
        cluster_id = get_cluster_id(backup_id)
        info = get_cluster_info_assert_exists(cluster_id, cluster_type, include_deleted=True)
        return get_cluster_backup_by_id(info, backup_id)


@API.resource('/mdb/<ctype:cluster_type>/<version>/clusters/<cluster_id>:backup')
class BackupClusterV1(MethodView):
    """
    Create backup for database cluster
    """

    @marshal.with_schema(OperationSchemaV1)
    @check_auth(
        folder_resolver=get_folder_by_cluster_id,
        resource=Resource.BACKUP,
        operation=DbaasOperation.CREATE,
    )
    @supports_idempotence
    def post(self, cluster_id: str, cluster_type: str, **_) -> dict:
        """
        Create backup for cluster
        """
        with metadb.commit_on_success():
            # lock cluster, cause don't want
            # concurrent create backup calls
            cluster_info = locked_cluster_info(cluster_id=cluster_id, cluster_type=cluster_type)

            try:
                handler = get_request_handler(cluster_type, Resource.BACKUP, DbaasOperation.CREATE)
            except UnsupportedHandlerError:
                raise DbaasNotImplementedError('Manual backups for {0} not implemented'.format(cluster_type))

            check_cluster_not_in_status(cluster_info, ClusterStatus.stopped)
            operation = handler(cluster_info)
            metadb.complete_cluster_change(cluster_id)

            return render_operation_v1(operation)
