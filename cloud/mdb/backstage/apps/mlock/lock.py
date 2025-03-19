import django.db


LOCK_LIST_QUERY = """SELECT * FROM code.get_locks(null, 500, 0)"""   # holder, limit, offset

FILTER_LOCK_QUERY = """
    SELECT
        l.lock_ext_id AS lock_ext_id,
        l.holder AS holder,
        l.reason AS reason,
        l.create_ts AS create_ts
    FROM
        mlock.locks l
        JOIN mlock.object_locks ol ON (l.lock_id = ol.lock_id)
        JOIN mlock.objects o ON (ol.object_id = o.object_id)
    WHERE
        object_name = %(fqdn)s
    GROUP BY l.lock_id
    ORDER BY l.create_ts
    LIMIT 1
"""


class LockList(list):
    @property
    def query(self):
        return LOCK_LIST_QUERY


class Lock:
    def __init__(
        self,
        lock_ext_id,
        holder,
        reason,
        create_ts,
        objects=None,
    ):
        self.pk = lock_ext_id
        self.lock_ext_id = lock_ext_id
        self.holder = holder
        self.reason = reason
        self.create_ts = create_ts
        self.objects = objects or []

    @classmethod
    def get_locks(self):
        locks = LockList()
        with django.db.connections['mlock_db'].cursor() as cursor:
            cursor.execute(LOCK_LIST_QUERY)
            for row in cursor.fetchall():
                lock_ext_id, holder, reason, create_ts, objects = row
                locks.append(Lock(
                    lock_ext_id=lock_ext_id,
                    holder=holder,
                    reason=reason,
                    create_ts=create_ts,
                    objects=objects,
                ))
        return locks

    @classmethod
    def get_lock_for_fqdn(self, fqdn):
        with django.db.connections['mlock_db'].cursor() as cursor:
            cursor.execute(FILTER_LOCK_QUERY, {'fqdn': fqdn})
            row = cursor.fetchone()
            if row:
                lock_ext_id, holder, reason, create_ts = row
                return Lock(
                    lock_ext_id=lock_ext_id,
                    holder=holder,
                    reason=reason,
                    create_ts=create_ts,
                )
            else:
                return None
