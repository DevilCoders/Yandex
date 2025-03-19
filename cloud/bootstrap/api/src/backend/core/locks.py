"""Locks logic (current implementation does not use external storage)"""

import datetime
import sys
from typing import List, Type

import psycopg2

from bootstrap.common.rdbms.exceptions import DbError, RecordNotFoundError
from bootstrap.common.rdbms.db import Db
from .instances import get_instances_as_type

from .types import Lock, LockedInstance, BaseInstance
from .utils import decorate_exported

__all__ = [
    # extra errors
    "CanNotLockError",

    # api functions
    "get_lock",
    "get_all_locks",
    "try_lock",
    "unlock",
    "extend",
]


class CanNotLockError(DbError):
    pass


def _get_base_hosts(host_fqdns: List[str], *, db: Db) -> List[Type[BaseInstance]]:
    """Get hosts from list of fqdns"""
    return get_instances_as_type(host_fqdns, klass=BaseInstance, db=db)


def _cleanup_expired_locks(*, db: Db) -> None:
    """Remove all expired locks. This function is dangerous as commit changes"""
    now = datetime.datetime.now()

    cursor = db.conn.cursor()
    cursor.execute("DELETE FROM locks WHERE expired_at < %s", (now,))
    db.conn.commit()


def get_lock(id: int, *, db: Db) -> Lock:
    """Get lock"""
    if not db.record_exists("locks", id):
        raise RecordNotFoundError("Lock <{}> is not found in database".format(id))

    lock = Lock(*db.select_one("locks", id))

    query = """
        SELECT instances.fqdn FROM instances
        INNER JOIN locked_instances ON
            locked_instances.instance_id = instances.id
        WHERE
            locked_instances.lock_id = %s
        ORDER BY
            instances.fqdn;
    """
    lock.hosts = db.select_by_condition(query, (lock.id,), one_column=True)

    return lock


def get_all_locks(*, db: Db) -> List[Lock]:
    """List all locks"""
    _cleanup_expired_locks(db=db)

    query = "SELECT locks.id FROM locks ORDER BY locks.id;"
    lock_ids = db.select_by_condition(query, (), one_column=True)

    # FIXME: very unoptimal !!!
    locks = [get_lock(lock_id, db=db) for lock_id in lock_ids]

    return locks


def try_lock(hosts: List[str], owner: str, description: str, hb_timeout: int, *, db: Db) -> Lock:
    """Try to lock specified hosts"""
    _cleanup_expired_locks(db=db)

    hosts = _get_base_hosts(hosts, db=db)

    # create lock
    expired_at = datetime.datetime.now() + datetime.timedelta(seconds=hb_timeout)
    lock = Lock(None, owner, description, hb_timeout, expired_at, [host.fqdn for host in hosts])

    # add lock to db
    db.insert_one("locks", lock)

    # add hosts locked by lock to db
    locked_instances = [LockedInstance(host.id, lock.id) for host in hosts]
    for locked_host in locked_instances:
        try:
            db.insert_one("locked_instances", locked_host)
        except psycopg2.errors.UniqueViolation:
            db.conn.rollback()

            locked_host_fqdn = [host.fqdn for host in hosts if host.id == locked_host.instance_id][0]

            query = """
                SELECT locks.description, locks.owner FROM instances
                INNER JOIN locked_instances ON
                    locked_instances.instance_id = instances.id
                INNER JOIN locks ON
                    locked_instances.lock_id = locks.id
                WHERE
                    instances.fqdn = %s;
            """
            lock_descr, lock_owner = db.select_by_condition(query, (locked_host_fqdn,))[0]

            raise CanNotLockError("Host {} is already locked by <{}>: lock description <{}>".format(
                locked_host_fqdn, lock_owner, lock_descr
            ))

    db.conn.commit()

    return lock


def unlock(id: int, *, db: Db) -> None:
    """Unlock specifed lock"""
    lock = get_lock(id, db=db)

    db.delete_one("locks", lock)

    db.conn.commit()


def extend(id: int, *, db: Db) -> Lock:
    """Extend specified lock"""
    _cleanup_expired_locks(db=db)

    lock = get_lock(id, db=db)
    lock.expired_at = datetime.datetime.now() + datetime.timedelta(seconds=lock.hb_timeout)

    db.update_one("locks", ["expired_at", ], lock)

    db.conn.commit()

    return lock


decorate_exported(sys.modules[__name__])
