"""Salt roles (CLOUD-28882)"""

import sys
from typing import List

from bootstrap.common.rdbms.db import Db
from bootstrap.common.rdbms.exceptions import RecordNotFoundError, RecordAlreadyInDbError

from .types import SaltRole
from .utils import decorate_exported

__all__ = [
    "get_all_salt_roles",

    "get_salt_role",
    "add_salt_role",
    "delete_salt_role",
]


def get_all_salt_roles(*, db: Db) -> List[SaltRole]:
    query = "SELECT * FROM salt_roles;"

    salt_roles = [SaltRole(*columns) for columns in db.select_by_condition(query, ())]

    return salt_roles


def _get_salt_role(name: str, *, db: Db) -> SaltRole:
    if not db.record_exists("salt_roles", name, column_name="name"):
        raise RecordNotFoundError("Salt role <{}> is not found in database".format(name))

    query = "SELECT * FROM salt_roles WHERE name = %s;"

    salt_role = SaltRole(*db.select_by_condition(query, (name,), ensure_one_result=True))

    return salt_role


def get_salt_role(name: str, *, db: Db) -> SaltRole:
    return _get_salt_role(name=name, db=db)


def add_salt_role(*, name: str, db: Db) -> SaltRole:
    if db.record_exists("salt_roles", name, column_name="name"):
        raise RecordAlreadyInDbError("Salt role <{}> is already in database".format(name))

    salt_role = SaltRole(None, name)
    db.insert_one("salt_roles", salt_role)

    db.conn.commit()

    return salt_role


def delete_salt_role(name: str, *, db: Db) -> None:
    salt_role = _get_salt_role(name=name, db=db)

    db.delete_one("salt_roles", salt_role)

    db.conn.commit()

decorate_exported(sys.modules[__name__])
