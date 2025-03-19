"""
Pillar manipulation module
"""

import json

from dbaas_common import retry, tracing
from dbaas_common.dict import combine_dict
from .base_metadb import BaseMetaDBProvider
from .common import Change
from ..metadb import DatabaseConnectionError
from ..query import execute


def _pillar_get(cur, path, **kwargs):
    res = execute(cur, 'get_pillar', path=path, **kwargs)
    return res[0]['value'] if res and res[0]['value'] else {}


def _pillar_has_keys(cur, path, keys, **kwargs):
    pillar = _pillar_get(cur, path, **kwargs)
    return all([x in pillar for x in keys])


def _pillar_set(cur, cid, rev, path, value, pillar_keys):
    expanded = {}
    cur_point = expanded
    for i in path:
        cur_point[i] = {}
        cur_point = cur_point[i]
    for key, val in value.items():
        cur_point[key] = val
    merged = combine_dict(_pillar_get(cur, [], **pillar_keys), expanded)

    execute(
        cur, 'upsert_pillar', fetch=False, cluster_cid=cid, cluster_rev=rev, value=json.dumps(merged), **pillar_keys
    )


def _key_to_query_kwargs(key_type, key):
    kwargs = {
        'cid': None,
        'subcid': None,
        'shard_id': None,
        'fqdn': None,
    }
    if key_type not in kwargs:
        raise Exception('Invalid pillar key type: {type}'.format(type=key_type))
    kwargs[key_type] = key
    return kwargs


def pillar_without_keys(pillar, path, keys):
    dst_pillar = pillar.copy()
    cur_point = dst_pillar
    for i in path:
        cur_point = cur_point[i]
    for key in keys:
        del cur_point[key]
    return dst_pillar


class DbaasPillar(BaseMetaDBProvider):
    """
    DBaaS MetaDB pillar provider
    """

    def _add_change(self, key_type, key, path, action):
        """
        Custom change add helper
        """
        change_key = f'pillar.{key_type}:{key}.{".".join(path)}'
        self.add_change(Change(change_key, action, rollback=Change.noop_rollback))

    # pylint: disable=no-member,too-many-arguments
    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('MetaDB Pillar Exists')
    def exists(self, key_type, key, path, keys, values, force=False):
        """
        Set pillar value if path was empty
        """
        tracing.set_tag('metadb.pillar.key_type', key_type)
        tracing.set_tag('metadb.pillar.key', key)
        tracing.set_tag('metadb.pillar.path', path)
        tracing.set_tag('metadb.pillar.keys', keys)

        self._add_change(key_type, key, path, 'updated')
        kwargs = _key_to_query_kwargs(key_type, key)
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rev = None
                if force or not _pillar_has_keys(cur, path, keys, **kwargs):
                    if rev is None:
                        rev = self.lock_cluster(conn, self.task['cid'])
                    db_value = values() if callable(values) else values
                    if callable(values) and type(db_value) != tuple:
                        raise RuntimeError(
                            f'Values() should return tuple of values, but type(values())={type(db_value)}'
                        )
                    values_dict = {}
                    for index, vkey in enumerate(keys):
                        values_dict[vkey] = db_value[index]
                    _pillar_set(cur, self.task['cid'], rev, path, values_dict, kwargs)
                if rev is not None:
                    self.complete_cluster_change(conn, self.task['cid'], rev)

    # pylint: disable=no-member,too-many-arguments
    @retry.on_exception(DatabaseConnectionError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('MetaDB Pillar Remove')
    def remove(self, key_type, key, path, keys):
        tracing.set_tag('metadb.pillar.key_type', key_type)
        tracing.set_tag('metadb.pillar.key', key)
        tracing.set_tag('metadb.pillar.path', path)
        tracing.set_tag('metadb.pillar.keys', keys)

        kwargs = _key_to_query_kwargs(key_type, key)

        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                rev = None
                if _pillar_has_keys(cur, path, keys, **kwargs):
                    self._add_change(key_type, key, path, 'removed keys')
                    if rev is None:
                        rev = self.lock_cluster(conn, self.task['cid'])
                    pillar = pillar_without_keys(_pillar_get(cur, [], **kwargs), path, keys)
                    execute(
                        cur,
                        'upsert_pillar',
                        fetch=False,
                        cluster_cid=self.task['cid'],
                        cluster_rev=rev,
                        value=json.dumps(pillar),
                        **kwargs,
                    )
                    self.complete_cluster_change(conn, self.task['cid'], rev)

    def get(self, key_type, key, path):
        kwargs = _key_to_query_kwargs(key_type, key)
        with self.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                return _pillar_get(cur, path, **kwargs)
