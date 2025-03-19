"""
MetaDB host manipulation module
"""

from typing import List, NamedTuple

from dbaas_common import retry
from .base_metadb import BaseMetaDBProvider
from .common import Change
from ..metadb import DatabaseConnectionError
from ..query import execute


class MetaDBSecurityGroupInfo(NamedTuple):
    """
    Security Group info from meta db
    """

    network_id: str
    service_sg: str
    service_sg_hash: int
    service_sg_allow_all: bool
    user_sgs: List[str]


class MetadbSecurityGroup(BaseMetaDBProvider):
    """
    DBaaS MetaDB security group provider
    """

    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    def delete_sgroups(self, sg_type):
        """
        Add security group to Meta DB
        """
        cid = self.task['cid']
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rev = self.lock_cluster(conn, cid)
                execute(cur, 'delete_sgroups', fetch=False, cid=cid, type=sg_type)
                self.complete_cluster_change(conn, cid, rev)
                self.add_change(Change(f'metadb_security_group.{cid}.id', 'deleted', rollback=Change.noop_rollback))

    def add_service_sgroup(self, sg_ext_id: str, sg_hash: int, sg_allow_all):
        self._add_sgroups('service', [sg_ext_id], sg_hash, sg_allow_all)

    def add_user_sgroups(self, list_sgroups: List[str]):
        self._add_sgroups('user', list_sgroups, 0, False)

    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    def _add_sgroups(self, sg_type, list_sgroups, sg_hash: int, sg_allow_all: bool):
        """
        Add security group to Meta DB
        """
        cid = self.task['cid']
        sg_ext_ids = ','.join(list_sgroups)
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rev = self.lock_cluster(conn, cid)
                execute(
                    cur,
                    'add_sgroups',
                    fetch=False,
                    cid=cid,
                    sg_type=sg_type,
                    sg_ext_ids=sg_ext_ids,
                    sg_hash=sg_hash,
                    sg_allow_all=sg_allow_all,
                )
                self.complete_cluster_change(conn, cid, rev)
                self.add_change(Change(f'metadb_security_group.{cid}.id', 'add', rollback=Change.noop_rollback))

    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    def update_sg_hash(self, sg_id, sg_hash, sg_allow_all):
        """
        Set security group hash to Meta DB
        """
        cid = self.task['cid']
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rev = self.lock_cluster(conn, cid)
                execute(
                    cur,
                    'update_sgroup_hash',
                    fetch=False,
                    cid=cid,
                    sg_ext_id=sg_id,
                    sg_hash=sg_hash,
                    sg_allow_all=sg_allow_all,
                )
                self.complete_cluster_change(conn, cid, rev)
                self.add_change(Change(f'update_security_group_hash.{cid}.id', 'update', rollback=Change.noop_rollback))

    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    def get_sgroup_info(self, cid: str) -> MetaDBSecurityGroupInfo:
        """
        Gets security group id by cid from metadb
        """
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rows = execute(cur, 'get_sgroups_info', cid=cid)
                if not rows:
                    raise RuntimeError('can not get cluster with security group info for cid {cid}')
                service_sg = ""
                service_sg_hash = 0
                user_sgs = []
                service_sg_allow_all = False
                network_id = rows[0]['network_id']
                for row in rows:
                    sgroups = row['sgroups']
                    if row['sg_type'] == 'service':
                        if len(sgroups) != 1:
                            raise RuntimeError("expected only one service security group")
                        service_sg = sgroups[0]
                        service_sg_hash = row['sg_hash']
                        service_sg_allow_all = row['sg_allow_all']
                    elif row['sg_type'] == 'user':
                        user_sgs = sgroups
                    elif not row['sg_type']:
                        pass
                    else:
                        raise RuntimeError("invalid content of security group")

                return MetaDBSecurityGroupInfo(
                    network_id=network_id,
                    service_sg=service_sg,
                    user_sgs=user_sgs,
                    service_sg_hash=service_sg_hash,
                    service_sg_allow_all=service_sg_allow_all,
                )
