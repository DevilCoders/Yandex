"""
Module describes helpers for request context
"""

from flask import g, request
from typing import NamedTuple, Optional

from .helpers import copy_merge_dict
from .types import FolderIds, IdempotenceData


class _RevInGlobals(NamedTuple):
    cid: str
    rev: int


def get_x_request_id() -> Optional[str]:
    """
    get X-Request-Id
    """
    return request.environ.get('HTTP_X_REQUEST_ID', None)


def get_forwarded_remote_address() -> Optional[str]:
    """
    get forwarded remote address
    """
    return request.environ.get('HTTP_X_FORWARDED_FOR', None)


def get_forwarded_user_agent() -> Optional[str]:
    """
    get forwarded user agent
    """
    return request.environ.get('HTTP_X_FORWARDED_AGENT', None)


def get_rev(cid: str) -> int:
    """
    Get cluster revision stored in cotext
    """
    if not hasattr(g, 'rev'):
        raise RuntimeError('Revision not defined in globals')
    if g.rev.cid != cid:
        raise RuntimeError('Revision in globals belongs to different cluster: %r. Requested: %r' % (g.rev.cid, cid))
    return g.rev.rev


def set_rev(cid: str, rev: int) -> None:
    """
    Set cluster revision in context
    """
    g.rev = _RevInGlobals(cid=cid, rev=rev)


def get_idempotence_from_request() -> Optional[IdempotenceData]:
    """
    extract idempotence data from request
    """
    for item in IdempotenceData._fields:
        if not hasattr(g, item):
            return None
    return IdempotenceData(*(getattr(g, item) for item in IdempotenceData._fields))


def get_folder_ids_from_request() -> FolderIds:
    """
    Construct FolderIds from request
    """
    return FolderIds(folder_id=g.folder['folder_id'], folder_ext_id=g.folder['folder_ext_id'])


def get_cloud_ext_id_from_request() -> str:
    """
    get cloud_ext_id form request
    """
    return g.cloud['cloud_ext_id']


def get_auth_context() -> dict:
    """
    get auth context
    """
    return request.auth_context  # type: ignore


def get_user_request() -> Optional[dict]:
    """
    get user request
    """
    # Get JSON Body
    # optional, e.g. it is empty for DELETE /mdb/mysql/1.0/clusters/cluster_id/users/user_id
    json = request.get_json(silent=True)  # type: ignore
    if json is None:
        json = {}

    # Path
    # e.g. e.g./mdb/mysql/1.0/clusters/cluster_id/users/user_id
    args = request.view_args
    if args is None:
        args = {}

    # Additional Query Parameters
    # e.g. /mdb/mysql/1.0/clusters?folderId=folder1
    query = request.args.to_dict()
    if query is None:
        query = {}

    return copy_merge_dict(json, copy_merge_dict(args, query))
