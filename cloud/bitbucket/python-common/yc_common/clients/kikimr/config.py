"""Database endpoint configuration"""

import getpass
import os

from yc_common import config, constants
from yc_common.exceptions import Error
from yc_common.models import Model, StringType, BooleanType, IntType


class KikimrEndpointConfig(Model):
    host = StringType(required=True)
    ydb_host = StringType()  # FIXME: Deprecated, remove after remove from salt
    ydb_connection_timeout = IntType(default=5)
    ydb_stop_timeout = IntType(default=1)
    ydb_wait_session_timeout = IntType(default=5)
    ydb_session_pool_initial_size = IntType(default=10)
    ydb_session_pool_max_size = IntType(default=100)
    ydb_session_worker_count = IntType(default=4)
    ydb_prepare_timeout = IntType(default=constants.DATABASE_REQUEST_TIMEOUT)
    ydb_client_version = IntType(default=1)  # FIXME: Deprecated, remove after remove from salt
    ydb_auth_token_file = StringType()
    root_ssl_cert_file = StringType()

    # if empty string or "default-url" - use default metadata url.
    # if non empty string - use it as token URL (for underlay instances)
    # if is None - don't use metadata token (default)
    ydb_token_from_metadata = StringType()
    allow_autoprepare = BooleanType(default=True)
    database = StringType()
    root = StringType(regex="^/", required=True)
    max_select_rows = IntType(min_value=1, default=1000)

    # FIXME: Configure the timeouts
    request_timeout = IntType(min_value=1, default=constants.DATABASE_REQUEST_TIMEOUT)
    retry_timeout = IntType(min_value=1, default=constants.DATABASE_REQUEST_TIMEOUT)
    scheme_operation_timeout = IntType(min_value=1, default=600)

    warning_request_time = IntType(min_value=1, default=3)
    warning_period = IntType(min_value=1, default=3)

    enable_logging = BooleanType(default=False)
    enable_batching = BooleanType(default=True)

    # Enabled for all DB in process if load any config with true
    enable_trace_ydb_session_leak = BooleanType(default=True)
    disable_pool = BooleanType(default=False)

    def get_root_path(self):
        root_path = self.root.rstrip("/")
        vagrant_var_string = "/$vagrant/"

        # FIXME: Rewrite + very inefficient
        if vagrant_var_string in root_path:
            machine_id = _get_vagrant_machine_id()
            root_path = root_path.replace(vagrant_var_string, "/vagrant/" + machine_id + "/")

        if "$" in root_path:
            raise Error("KiKiMR root path contains an unbound variable.")

        return root_path


def get_database_config(database_id) -> KikimrEndpointConfig:
    return config.get_value("endpoints.kikimr." + database_id, model=KikimrEndpointConfig)


_VAGRANT_MACHINE_ID = None


def _get_vagrant_machine_id():
    global _VAGRANT_MACHINE_ID
    if _VAGRANT_MACHINE_ID is None:
        _VAGRANT_MACHINE_ID = _read_vagrant_machine_id()
    return _VAGRANT_MACHINE_ID


def _read_vagrant_machine_id():
    if getpass.getuser() == "vagrant":
        vagrant_path = "/vagrant"
    elif config.get_value("devel_mode", False):
        vagrant_path = "."
    else:
        raise Error("Unable to expand $vagrant variable in KiKiMR root path: we aren't running under Vagrant.")

    machine_id_path = os.path.join(vagrant_path, ".vagrant/machines/default/virtualbox/id")

    try:
        with open(machine_id_path) as machine_id_file:
            return machine_id_file.read().strip()
    except OSError as e:
        raise Error("Unable to get Vagrant machine ID from {!r}: {}.", machine_id_path, e.strerror)
