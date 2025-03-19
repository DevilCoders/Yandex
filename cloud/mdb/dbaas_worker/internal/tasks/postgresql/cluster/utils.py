from typing import Dict

from ....exceptions import UserExposedException
from ....utils import get_first_key
from ....providers.pgsync import PgSync, pgsync_cluster_prefix


class SyncExtensionsException(UserExposedException):
    """
    Sync extension operation failed. Showing this to user.
    """


def get_master_host(cid, pgsync: PgSync, hosts: Dict, zk_hosts: Dict):
    if len(hosts) == 1:
        return get_first_key(hosts)
    return pgsync.get_master(zk_hosts, pgsync_cluster_prefix(cid))
