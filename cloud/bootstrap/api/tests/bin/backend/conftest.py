import datetime
import os
import pytest
import socket
import subprocess
import tempfile
import yaml

from library.python import resource
from library.python.retry import retry_call, RetryConf
from yatest import common

from bootstrap.common.rdbms.db import DbConfig, Db
from bootstrap.db.admin.app import _populate, _erase
from bootstrap.api.core.constants import SUPPORTED_DB_VERSIONS

DB_VERSION = SUPPORTED_DB_VERSIONS[-1]

BOOTSTRAP_API_PORT = 23712  # FIXME: must conform to port from examples/api_config.yaml

SKIP_SETUP_API_VAR = "SKIP_SETUP_API"


@pytest.fixture(scope="session")
def bootstrap_db():
    """Initialize bootstrap db, raised in docker container (look ya.make)\

       Have to retry here, because after docker container is raised, database is still initializing."""

    # create db
    retry_conf = RetryConf(max_time=datetime.timedelta(seconds=60))
    db = retry_call(Db, (DbConfig(yaml.safe_load(resource.find("localdb.yaml"))), True), conf=retry_conf)

    _populate(db, resource.find("current.bootstrap.sql"), DB_VERSION)

    return db


@pytest.fixture(scope="function", autouse=True)
def _cleanup_bootstrap_db_data(bootstrap_db):
    yield

    bootstrap_db.conn.rollback()

    _erase(bootstrap_db)

    _populate(bootstrap_db, resource.find("current.bootstrap.sql"), DB_VERSION)


def _wait_bootstrap_api_raised() -> None:
    retry_conf = RetryConf(max_time=datetime.timedelta(seconds=5))
    s = socket.socket()
    retry_call(s.connect, (("localhost", BOOTSTRAP_API_PORT),), conf=retry_conf)


@pytest.fixture(scope="session")
def bootstrap_api():
    if not os.environ.get(SKIP_SETUP_API_VAR, None):
        with tempfile.TemporaryDirectory(prefix="yc-bootstrap-api-test") as tmpdir:
            config_file = os.path.join(tmpdir, "localapi.yaml")
            with open(config_file, "bw") as f:
                f.write(resource.find("localapi.yaml"))

            bootstrap_api_args = [
                common.binary_path("cloud/bootstrap/api/bin/backend/bootstrap.api"),
                "--config",
                config_file,
            ]
            p = subprocess.Popen(bootstrap_api_args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

            _wait_bootstrap_api_raised()

    yield None

    if not os.environ.get(SKIP_SETUP_API_VAR, None):
        p.kill()
