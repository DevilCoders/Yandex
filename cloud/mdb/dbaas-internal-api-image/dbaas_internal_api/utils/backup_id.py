"""
Backup.id workaround
"""

from ..core.id_generators import gen_id
from typing import Tuple


class MalformedGlobalBackupId(Exception):
    """
    Malformed backup id
    """


def encode_global_backup_id(cluster_id: str, backup_id: str) -> str:
    """
    Create global unique backup_id
    """
    # MDB-3319 - should encode it in cloud style
    return cluster_id + ':' + backup_id


def decode_global_backup_id(global_backup_id: str) -> Tuple[str, str]:
    """
    Cast global backup_id to local backup.id
    """
    try:
        cluster_id, backup_id = global_backup_id.split(':', 1)
        return cluster_id, backup_id
    except ValueError:
        raise MalformedGlobalBackupId('Malformed backup id: %s' % global_backup_id)


def generate_backup_id() -> str:
    """
    Generate backup_id
    """
    return gen_id('backup')
