"""Auxiliarily functions"""

import types
from typing import Any, Optional

from bootstrap.common.rdbms.db import Db
from bootstrap.common.rdbms.decorators import force_rollback
from bootstrap.common.rdbms.exceptions import RecordNotFoundError

from .decorators import flask_app_db, verify_and_update_conn


def decorate_exported(module):
    """Decorate all exported functions"""
    for func_name in getattr(module, "__all__"):
        func = getattr(module, func_name)
        if isinstance(func, types.FunctionType):
            setattr(module, func_name, flask_app_db(verify_and_update_conn(force_rollback(func))))


def ensure_record_exists(db: Db, table_name: str, value: Any, column_name: str = "id",
                         error_msg: Optional[str] = None) -> None:
    if error_msg is None:
        # some magic to convert table name to readable entity name
        entity_name = table_name.capitalize()
        if entity_name.endswith("s"):
            entity_name = entity_name[:-1]
        entity_name = entity_name.replace("_", " ")

        error_msg = "{} <{}> is not found in database".format(entity_name, value)

    if not db.record_exists(table_name, value, column_name=column_name):
        raise RecordNotFoundError(error_msg)
