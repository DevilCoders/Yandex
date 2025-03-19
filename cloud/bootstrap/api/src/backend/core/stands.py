"""Stands logic (CLOUD-28882)"""

import sys
from typing import Dict, List

from bootstrap.common.rdbms.db import Db
from bootstrap.common.rdbms.exceptions import RecordNotFoundError

from .types import Stand, StandClusterMap
from .utils import decorate_exported


__all__ = [
    "get_all_stands",

    "get_stand",
    "add_stand",
    "delete_stand",
]


def get_all_stands(*, db: Db) -> List[Stand]:
    query = "SELECT * FROM stands;"

    stands = [Stand(*columns) for columns in db.select_by_condition(query, ())]

    return stands


def _get_stand(name: str, *, db: Db) -> Stand:
    if not db.record_exists("stands", name, column_name="name"):
        raise RecordNotFoundError("Stand <{}> is not found in database".format(name))

    query = "SELECT * FROM stands WHERE name = %s;"

    stand = Stand(*db.select_by_condition(query, (name,), ensure_one_result=True))

    return stand


def get_stand(name: str, *, db: Db) -> Stand:
    return _get_stand(name=name, db=db)


def add_stand(*, name: str, db: Db) -> Stand:
    stand = Stand(None, name)
    db.insert_one("stands", stand)

    stand_cluster_map = StandClusterMap(None, name, None, None, None, None)
    db.insert_one("stand_cluster_maps", stand_cluster_map)

    db.conn.commit()

    return stand


def update_stand(name: str, *, data: Dict, db: Db) -> Stand:
    """Nothing to do at the moment"""

    stand = _get_stand(name=name, db=db)

    db.conn.commit()

    return stand


def delete_stand(name: str, *, db: Db) -> None:
    stand = _get_stand(name=name, db=db)

    db.delete_one("stands", stand)

    db.conn.commit()


decorate_exported(sys.modules[__name__])
