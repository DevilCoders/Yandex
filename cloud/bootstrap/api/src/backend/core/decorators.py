import functools

import flask

from .constants import SUPPORTED_DB_VERSIONS
from .exceptions import DbInMigrationError, UnsupportedDbVersionError


def flask_app_db(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        if not kwargs.get("db"):
            kwargs["db"] = flask.current_app.db
        return func(*args, **kwargs)
    return wrapper


def verify_and_update_conn(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        db = kwargs.get("db")

        db.verify_and_update_conn()  # force connect if not connected at the moment

        if db.migrating_to_version is not None:
            raise DbInMigrationError("Underlying database is migrating to version <{}>".format(db.migrating_to_version))
        if db.version not in SUPPORTED_DB_VERSIONS:
            fmtmsg = "Underlying database version <{}> is not supported (should be one of: {})"
            raise UnsupportedDbVersionError(fmtmsg.format(db.version, ", ".join(SUPPORTED_DB_VERSIONS)))
        return func(*args, **kwargs)
    return wrapper
