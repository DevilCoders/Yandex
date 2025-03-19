"""
Base type for pillar
"""

from copy import deepcopy
from typing import Dict

from psycopg2.extensions import ISQLQuote
from psycopg2.extras import Json

from ..utils.helpers import merge_dict
from .crypto import encrypt_bytes


def set_e2e_cluster(pillar: dict) -> None:
    """
    Set True for cluster created for testing purposes.
    """
    pillar['data'].setdefault('mdb_metrics', {})
    pillar['data'].setdefault('billing', {})
    pillar['data'].setdefault('mdb_health', {})
    pillar['data']['use_yasmagent'] = False
    pillar['data']['suppress_external_yasmagent'] = True
    pillar['data']['ship_logs'] = False
    pillar['data']['mdb_metrics']['enabled'] = False
    pillar['data']['billing']['ship_logs'] = False
    pillar['data']['mdb_health']['nonaggregatable'] = True


class BasePillar:
    """
    BasePillar know how convert itself to DB
    """

    __slots__ = ['_pillar']

    def __init__(self, pillar: Dict) -> None:
        self._pillar = pillar

    def as_dict(self) -> Dict:
        """
        Return pillar dict
        """
        return deepcopy(self._pillar)

    def __conform__(self, proto):
        """
        Tell psycopg2 that we this type
        know how it should be adapted
        """
        if proto is ISQLQuote:
            return self

    def getquoted(self):
        """
        Convert pillar to db object
        """
        # Expect that Json
        # adapter know how convert UUID
        return Json(self._pillar).getquoted()

    @property
    def s3_bucket(self) -> str:
        """
        Get S3 bucket
        """
        return self._pillar['data']['s3_bucket']

    @s3_bucket.setter
    def s3_bucket(self, name: str) -> None:
        """
        Set S3 bucket
        """
        self._pillar['data']['s3_bucket'] = name

    def set_cluster_private_key(self, key: bytes) -> None:
        """
        Set cluster private key
        """
        self._pillar['data']['cluster_private_key'] = encrypt_bytes(key)

    def set_e2e_cluster(self) -> None:
        """
        Set True for cluster created for testing purposes.
        """
        set_e2e_cluster(self._pillar)


class BackupPillar(BasePillar):
    """
    BackupPillar support also special options for backup
    """

    @property
    def backup_start(self) -> Dict:
        """
        Return backup window start
        """
        return deepcopy(self._pillar['data'].get('backup', {}).get('start', {}))

    def update_backup_start(self, backup_start) -> None:
        """
        Update backup window start
        """
        self._pillar['data'].setdefault('backup', {}).setdefault('start', {})
        merge_dict(self._pillar['data']['backup']['start'], backup_start)


class BackupAndAccessPillar(BackupPillar):
    """
    BackupAndAccessPillar support also special options for access
    """

    @property
    def access(self) -> Dict:
        """
        Return access options
        """
        return self._pillar['data'].get('access', {})

    def update_access(self, access: Dict) -> None:
        """
        Update set access options
        """
        if not access:
            return
        self._pillar['data'].setdefault('access', {})
        merge_dict(self._pillar['data']['access'], access)
