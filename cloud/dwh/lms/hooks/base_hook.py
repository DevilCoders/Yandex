"""Base class for all hooks"""
import logging
import os
from library.python.vault_client.instances import Production as VaultClient
from library.python.vault_client.errors import ClientError

from cloud.dwh.lms.exceptions import LMSException, InvalidConnIdError
from cloud.dwh.lms.models.connection import Connection
from cloud.dwh.lms import config

from cloud.dwh.lms.utils.log.logging_mixin import LoggingMixin

log = logging.getLogger(__name__)


class BaseHook(LoggingMixin):
    """
    Abstract base class for hooks, hooks are meant as an interface to
    interact with external systems. MySqlHook, HiveHook, PigHook return
    object that can handle the connection and interaction to specific
    instances of these systems, and expose consistent methods to interact
    with them.
    """
    @classmethod
    def get_connection(cls, conn_id: str) -> Connection:
        """
        Get random connection selected from all connections configured with this connection id.
        :param conn_id: str
        :return: connection
        """
        print(f"getting connection for {conn_id}")
        oauth_token = config.YAV_OAUTH_TOKEN or os.environ.get("YAV_OAUTH_TOKEN", None)
        if not oauth_token:
            raise LMSException("yav_secret_version argument or YAV_OAUTH_TOKEN env var must be provided")
        try:
            yav = VaultClient(authorization=oauth_token).get_version(conn_id)["value"]
        except ClientError:
            raise InvalidConnIdError(f"Could not find conn_id={conn_id} in yav")

        conn = Connection(
            conn_id=conn_id,
            conn_type=yav["conn_type"],
            host=yav["host"],
            port=yav["port"],
            login=yav["login"],
            schema=yav["database"],
            password=yav["password"],
            extra=yav.get("extra", None)
        )
        if conn.host:
            print("Using connection to: %s", conn.log_info())
        return conn

    def get_hook(self, conn_id: str) -> "BaseHook":
        """
        Returns default hook for this connection id.
        :param conn_id: connection id
        :return: default hook for this connection
        """
        # TODO: set method return type to BaseHook class when on 3.7+.
        #  See https://stackoverflow.com/a/33533514/3066428
        connection = self.get_connection(conn_id)
        return connection.get_hook()

    def get_conn(self):
        """Returns connection for the hook."""
        raise NotImplementedError()
