# -*- coding: utf-8 -*-
"""
DBaaS Internal API pillar functions
"""
import json
from typing import Dict, List  # noqa

from . import metadb
from ..core.crypto import encrypt_bytes
from ..core.id_generators import gen_id
from ..core.types import CID
from .helpers import copy_merge_dict, merge_dict
from .types import Pillar


def get_s3_bucket(pillar: Pillar) -> str:
    """
    Extract S3 bucket name from pillar
    """
    return pillar['data']['s3_bucket']


def get_s3_bucket_for_delete(pillar: Pillar) -> str:
    """
    Extract S3 bucket from pillar and verify that we can delete it
    """
    bucket = get_s3_bucket(pillar)
    if bucket == 'dbaas':
        raise RuntimeError('Try delete dbaas bucket!')
    return bucket


def set_s3_bucket(pillar: Pillar, name: str):
    """
    Set S3 bucket name in pillar
    """
    merge_dict(pillar, {'data': {'s3_bucket': name}})


def add_cluster_private_key(pillar: Pillar, private_key: bytes):
    """
    Add cluster private key to pillar and encrypt it
    """
    pillar['data']['cluster_private_key'] = encrypt_bytes(private_key)


def make_restore_pillar(source_pillar: Pillar) -> Pillar:
    """
    Return 'stripped' pillar
    """
    return {'data': {'restore-from-pillar-data': source_pillar['data']}}


def store_target_pillar(new_cluster_id: CID, source_cluster_pillar: Pillar) -> str:
    """
    Get source cluster pillar and store it in target_pillar
    """
    target_id = gen_id('target_pillar_id')

    metadb.add_target_pillar(
        target_id=target_id, cid=new_cluster_id, value=json.dumps(make_restore_pillar(source_cluster_pillar))
    )
    return target_id


def get_cluster_pillar(cid: CID) -> Pillar:
    """
    Return cluster pillar
    """
    return metadb.get_cluster(cid=cid)['value']


def make_merged_pillar(src_pillar: dict, new_pillar: dict) -> dict:
    """
    merge new_pillar onto src_pillar
    """
    return copy_merge_dict(src_pillar, new_pillar)


class SubPillar:
    """
    Holds link to pillar part

    Subclass must redefine _path attribute
    """

    __slots__ = ['_pillar_part', '_parent']
    _path = []  # type: List[str]

    # parent should inherit BasePillar, but mypy fails on it
    def __init__(self, parent_pillar: Dict, parent) -> None:
        _pp = parent_pillar
        for key in self._path:
            sub_pillar = _pp.get(key)
            if sub_pillar is None:
                _pp[key] = {}
                sub_pillar = _pp[key]
            _pp = sub_pillar
        if _pp is parent_pillar:
            raise RuntimeError('{0} hold same link as parent, path is {1}'.format(self, self._path))
        self._pillar_part = _pp
        self._parent = parent


class UninitalizedPillarSection(Exception):
    """
    Try access to uninitialized pillar section
    """
