"""Various migration tests"""

from typing import Optional

from common import bootstrap_api_req
from conftest import DB_VERSION


_BAD_VERSION = "1.2.3"


def _set_versions(bootstrap_db, version: Optional[str], migrating_to_version: Optional[str] = None):
    cur = bootstrap_db.conn.cursor()
    cur.execute("UPDATE scheme_info SET version = %s, migrating_to_version = %s;", (
        version, migrating_to_version
    ))
    bootstrap_db.conn.commit()
    bootstrap_db.drop_active_connections()


def test_migration_api_response(bootstrap_db, bootstrap_api):
    # change version to unsupported
    _set_versions(bootstrap_db, version=_BAD_VERSION)
    bootstrap_api_req("get", "v1/locks", expected_json={
        "code": 503,
        "data": None,
        "error_message": ("UnsupportedDbVersionError: Underlying database version <{}> is not supported (should be one "
                          "of: {})".format(_BAD_VERSION, DB_VERSION)),
        "status": "error"
    })

    # change back go to good version
    _set_versions(bootstrap_db, version=DB_VERSION)
    bootstrap_api_req("get", "v1/locks")

    # change to migration
    _set_versions(bootstrap_db, version=DB_VERSION, migrating_to_version=_BAD_VERSION)
    bootstrap_api_req("get", "v1/locks", expected_json={
        "code": 503,
        "data": None,
        "error_message": "DbInMigrationError: Underlying database is migrating to version <{}>".format(
            _BAD_VERSION
        ),
        "status": "error"
    })

    # complete migration
    _set_versions(bootstrap_db, version=DB_VERSION)
    bootstrap_api_req("get", "v1/locks")
