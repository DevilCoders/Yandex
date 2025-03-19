"""Database driver layer"""

import dataclasses
from enum import Enum
import logging
import json
from typing import Any, List, Optional, Tuple, Union

import psycopg2
import psycopg2.sql
from psycopg2.extras import MinTimeLoggingConnection
from schematics.models import Model
from schematics.types import IntType, StringType, BooleanType, ModelType

from bootstrap.common.config import BootstrapConfigMixin
from bootstrap.common.exceptions import BootstrapAssertionError
from bootstrap.common.rdbms.exceptions import RecordNotFoundError, MultipleRecordsFoundError


_AVAIL_SSL_MODES = ["disable", "allow", "prefer", "require", "verify-ca", "verify-full"]


def _convert_value_for_query(value: Any) -> Any:
    """Not all natural python types automatically converted to types accepted by psycopg2"""
    if isinstance(value, dict):
        return json.dumps(value)
    if isinstance(value, Enum):
        return value.value
    return value


class QueryLogConfig(Model):
    enabled = BooleanType(default=False)
    mintime = IntType(default=0)


class DbConfig(Model, BootstrapConfigMixin):
    _CONFIG_FILE = "/etc/yc/bootstrap/db/admin/config.yaml"
    _CONFIG_ARGPARSE_NAME = "db-config"
    _SECRET_CONFIG_FILE = "/etc/yc/bootstrap/db/admin/secret_config.yaml"
    _SECRET_CONFIG_ARGPARSE_NAME = "db-secret-config"

    host = StringType(required=True)
    port = IntType(required=True, min_value=1, max_value=65535)
    dbname = StringType(required=True)
    user = StringType(required=True)
    password = StringType(required=True)

    # optional stuff
    sslmode = StringType(default="verify-full", choices=_AVAIL_SSL_MODES)
    query_log = ModelType(QueryLogConfig, default=QueryLogConfig)


class BaseDb:
    @dataclasses.dataclass
    class TableColumnInfo:
        name: str
        autofilled: bool
        primary_key: bool

    @dataclasses.dataclass
    class TableInfo:
        name: str
        columns: List["BaseDb.TableColumnInfo"]

        @property
        def all_column_names(self):
            return [column.name for column in self.columns]

        @property
        def pkey_column_names(self):
            return [column.name for column in self.columns if column.primary_key]

        @property
        def autofilled_column_names(self):
            return [column.name for column in self.columns if column.autofilled]

    def __init__(self, db_config: DbConfig, connect: bool = False, logger: logging.Logger = None):
        self.config = db_config
        self.logger = logger

        self.bootstrapped = False
        self.version = None
        self.migrating_to_version = None

        self._conn = None
        if connect:
            self.__init_conn()

        self.__cached_table_info = dict()

    def __init_conn(self):
        additional_kwargs = {}
        if self.config.query_log.enabled:
            additional_kwargs["connection_factory"] = MinTimeLoggingConnection

        self._conn = psycopg2.connect(
            host=self.config.host,
            port=self.config.port,
            dbname=self.config.dbname,
            user=self.config.user,
            password=self.config.password,
            # FIXME: specify this fields in config?
            target_session_attrs="read-write",
            sslmode=self.config.sslmode,
            connect_timeout=10,
            **additional_kwargs,
        )
        if self.config.query_log.enabled:
            self._conn.initialize(self.logger, self.config.query_log.mintime)

        self.__cached_table_info = dict()

        self.update_versions_info()

    def update_versions_info(self) -> None:
        self.bootstrapped = self._table_exists("scheme_info")

        if self.bootstrapped:
            cursor = self.conn.cursor()
            cursor.execute("SELECT version, migrating_to_version FROM public.scheme_info;")
            self.version, self.migrating_to_version = cursor.fetchone()
        else:
            self.version, self.migrating_to_version = None, None

    def _table_exists(self, table_name: str) -> bool:
        """Check if table exists"""
        cursor = self.conn.cursor()
        cursor.execute("SELECT 1 FROM pg_tables WHERE tablename = %s;", (table_name,))
        return cursor.fetchone() is not None

    def _table_info(self, table_name: str, use_cache: bool = True) -> "BaseDb.TableInfo":
        """Return list of columns"""
        if (not use_cache) or (table_name not in self.__cached_table_info):
            cursor = self.conn.cursor()

            # get table primary keys
            query = """
                SELECT kcu.column_name from information_schema.table_constraints as tco, information_schema.key_column_usage as kcu
                WHERE
                    kcu.table_schema = 'public' AND
                    kcu.table_name = %s AND
                    kcu.constraint_name = tco.constraint_name AND
                    kcu.constraint_schema = tco.constraint_schema AND
                    kcu.constraint_name = tco.constraint_name AND
                    tco.constraint_type = 'PRIMARY KEY';
            """
            cursor.execute(query, (table_name,))
            records = cursor.fetchall()
            table_primary_keys = {record[0] for record in records}

            # get basic columns info
            query = """
                SELECT column_name, column_default FROM information_schema.columns
                WHERE
                    table_schema = 'public' AND
                    table_name = %s
                ORDER BY ordinal_position;
            """
            cursor.execute(query, (table_name,))
            records = cursor.fetchall()

            table_columns = []
            for name, default in records:
                autofilled = False
                if default and default.startswith("nextval"):
                    autofilled = True
                table_columns.append(BaseDb.TableColumnInfo(name, autofilled, name in table_primary_keys))

            self.__cached_table_info[table_name] = BaseDb.TableInfo(table_name, table_columns)

        return self.__cached_table_info[table_name]

    @property
    def conn(self):
        if not self._conn:
            self.__init_conn()
        return self._conn

    def verify_and_update_conn(self) -> None:
        """Reopen broken connections (FIXME: make better broken connection detection)"""
        # check for basic stuff
        if (not self.conn) or self.conn.closed:
            self.__init_conn()

        # some magic to detect if connection is really closed
        try:
            cursor = self.conn.cursor()
            cursor.execute("SELECT 1;")
        except psycopg2.OperationalError:
            self.__init_conn()

    @property
    def url(self) -> str:
        """Return connection url, accepted by psql"""
        return "postgresql://{user}:{password}@{host}:{port}/{dbname}".format(
            user=self.config.user,
            password="PASSWORD_IS_HIDDEN",
            host=self.config.host,
            port=self.config.port,
            dbname=self.config.dbname,
        )

    def drop_active_connections(self):
        """CLOUD-28882: drop active connections (currently used in migration)."""
        self.conn.commit()  # have to add to renew transaction

        cursor = self.conn.cursor()
        query = """
            SELECT pg_terminate_backend(pid) FROM pg_stat_activity
            WHERE
                datname = %s AND
                usename = %s AND
                pid != pg_backend_pid();
        """
        cursor.execute(query, (self.config.dbname, self.config.user))

        self.conn.commit()  # FIXME: do we need it


class Db(BaseDb):
    def __get_obj_column(self, obj: object, column_name: str):
        """Get column value from obj"""
        if hasattr(obj, column_name):
            value = getattr(obj, column_name)
        else:
            value = getattr(obj, "get_{}".format(column_name))(self)

        return _convert_value_for_query(value)

    def get_id_by_name(self, table_name: str, name: Optional[str], column_name: str = "name") -> Optional[int]:
        # do not have referenced entity
        if name is None:
            return None

        # have referenced entity
        cur = self.conn.cursor()
        query = "SELECT id FROM {} WHERE {} = %s;".format(table_name, column_name)
        cur.execute(query, (name, ))
        return cur.fetchone()[0]

    def record_exists(self, table_name: str, value: Any, column_name: str = "id") -> bool:
        """Check if record exists"""
        cur = self.conn.cursor()

        cur.execute("SELECT COUNT(*) FROM {} WHERE {} = %s;".format(table_name, column_name), (value,))

        return cur.fetchone()[0] > 0

    def record_exists_ext(self, table_name: str, column_names: List[str], values: List[Any]) -> bool:
        """Check if record exists with multiple conditions"""
        condition = " AND ".join(["{} = %s".format(column_name) for column_name in column_names])
        query = "SELECT COUNT(*) FROM {} WHERE {};".format(table_name, condition)

        cur = self.conn.cursor()
        cur.execute(query, values)

        return cur.fetchone()[0] > 0

    def select_one(self, table_name: str, id: Union[int, str], column_name: str = "id") -> Tuple:
        """Select one record from specified table"""
        cur = self.conn.cursor()

        cur.execute("SELECT * FROM {} WHERE {} = %s;".format(table_name, column_name), (id,))

        record = cur.fetchone()
        if not record:
            raise RecordNotFoundError("Object with id <{}> is not found in table <{}>".format(id, table_name))

        return record

    def select_all(self, table_name: str) -> List[Tuple]:
        cur = self.conn.cursor()

        cur.execute("SELECT * FROM {};".format(table_name), (id,))

        return cur.fetchall()

    def select_by_condition(self, query: str, query_params: Tuple, one_column: bool = False,
                            ensure_one_result: bool = False) -> Union[List[Any], Any]:
        cur = self.conn.cursor()

        cur.execute(query, query_params)

        records = cur.fetchall()
        if one_column:
            if records and (len(records[0]) > 1):
                raise BootstrapAssertionError("Got more than one colmun in query <{}>".format(query))
            records = [record[0] for record in records]

        if ensure_one_result:
            if len(records) == 0:
                raise RecordNotFoundError(
                    "Found 0 objects when selecting by condition <{} ({})>".format(
                        query, query_params
                    )
                )
            if len(records) > 1:
                raise MultipleRecordsFoundError(
                    "Found {} objects while selecting by condition <{} ({})>".format(
                        len(records), query, query_params
                    )
                )

            return records[0]

        return records

    def update_many(self, table_name: str, columns: List[str], objs: List[object]) -> None:
        """Update multiple records in specified table"""
        # some checks
        if not columns:
            raise BootstrapAssertionError("Got empty list of table columns when updating <{}>".format(table_name))
        unknown_columns = set(columns) - set(self._table_info(table_name).all_column_names)
        if unknown_columns:
            raise BootstrapAssertionError(
                "Columns <{}> not found in table <{}>".format(",".join(unknown_columns), table_name)
            )

        # generate query
        pkey_column_names = self._table_info(table_name).pkey_column_names
        set_expression = ", ".join("{} = %s".format(column) for column in columns)
        condition_expression = " AND ".join("{} = %s".format(column_name) for column_name in pkey_column_names)
        query = "UPDATE {} SET {} WHERE {}".format(table_name, set_expression, condition_expression)

        # generat data for update
        update_data = []
        for obj in objs:
            obj_update_data = tuple(
                self.__get_obj_column(obj, column) for column in columns + pkey_column_names
            )
            update_data.append(obj_update_data)

        # execute
        cur = self.conn.cursor()
        cur.executemany(query, update_data)

    def update_one(self, table_name: str, columns: List[str], obj: object) -> None:
        self.update_many(table_name, columns, [obj])

    def __generate_insert_query_str(self, table_name: str) -> Tuple[str, List[str], List[str]]:
        """Generate insert query and list of table columns for insert"""
        all_columns = self._table_info(table_name).all_column_names
        autofilled_columns = self._table_info(table_name).autofilled_column_names

        # create insert part
        insert_columns = [column for column in all_columns if column not in autofilled_columns]
        insert_part = "({columns_str}) VALUES ({values_str})".format(
            columns_str=", ".join(insert_columns),
            values_str=", ".join(["%s"] * len(insert_columns)),
        )

        # create returning autofillid stuff part
        if autofilled_columns:
            returning_part = "RETURNING {}".format(", ".join(autofilled_columns))
        else:
            returning_part = ""

        query = "INSERT INTO {table_name} {insert_part} {returning_part};".format(
            table_name=table_name, insert_part=insert_part, returning_part=returning_part
        )

        return query, insert_columns, autofilled_columns

    def insert_one(self, table_name: str, obj: object) -> None:
        """Insert object to database. Set `obj` fields, autofilled by db (such as autoincrement fields)"""
        query, insert_columns, autofilled_columns = self.__generate_insert_query_str(table_name)
        insert_data = [self.__get_obj_column(obj, column) for column in insert_columns]

        cur = self.conn.cursor()
        cur.execute(query, insert_data)

        if autofilled_columns:
            for column, column_value in zip(autofilled_columns, cur.fetchone()):
                setattr(obj, column, column_value)

    def delete_one(self, table_name, obj: object) -> None:
        """Delete specified object (by primary key)"""
        pkey_columns = self._table_info(table_name).pkey_column_names
        if not pkey_columns:
            raise BootstrapAssertionError("Try to delete object from table <{}> without primary key".format(table_name))

        conditions = " AND ".join("{} = %s".format(column) for column in pkey_columns)
        query = "DELETE FROM {table_name} WHERE {conditions};".format(table_name=table_name, conditions=conditions)
        query_data = [getattr(obj, column) for column in pkey_columns]

        cur = self.conn.cursor()
        cur.execute(query, query_data)
