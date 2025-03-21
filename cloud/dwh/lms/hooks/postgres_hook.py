import os
from contextlib import closing

import psycopg2
import psycopg2.extensions
import psycopg2.extras

from cloud.dwh.lms.hooks.dbapi_hook import DbApiHook


class PostgresHook(DbApiHook):
    conn_name_attr = 'postgres_conn_id'
    default_conn_name = 'postgres_default'
    supports_autocommit = True

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.schema = kwargs.pop("schema", None)
        self.connection = kwargs.pop("connection", None)
        self.conn = None

    def _get_cursor(self, raw_cursor):
        _cursor = raw_cursor.lower()
        if _cursor == 'dictcursor':
            return psycopg2.extras.DictCursor
        if _cursor == 'realdictcursor':
            return psycopg2.extras.RealDictCursor
        if _cursor == 'namedtuplecursor':
            return psycopg2.extras.NamedTupleCursor
        raise ValueError('Invalid cursor passed {}'.format(_cursor))

    def get_conn(self):

        conn_id = getattr(self, self.conn_name_attr)
        conn = self.connection or self.get_connection(conn_id)

        # check for authentication via AWS IAM
        if conn.extra_dejson.get('iam', False):
            conn.login, conn.password, conn.port = self.get_iam_token(conn)

        conn_args = dict(
            host=conn.host,
            user=conn.login,
            password=conn.password,
            dbname=self.schema or conn.schema,
            port=conn.port)
        raw_cursor = conn.extra_dejson.get('cursor', False)
        if raw_cursor:
            conn_args['cursor_factory'] = self._get_cursor(raw_cursor)
        # check for ssl parameters in conn.extra
        for arg_name, arg_val in conn.extra_dejson.items():
            if arg_name in ['sslmode', 'sslcert', 'sslkey',
                            'sslrootcert', 'sslcrl', 'application_name',
                            'keepalives_idle']:
                conn_args[arg_name] = arg_val

        self.conn = psycopg2.connect(**conn_args)
        return self.conn

    def copy_expert(self, sql, filename):
        """
        Executes SQL using psycopg2 copy_expert method.
        Necessary to execute COPY command without access to a superuser.
        Note: if this method is called with a "COPY FROM" statement and
        the specified input file does not exist, it creates an empty
        file and no data is loaded, but the operation succeeds.
        So if users want to be aware when the input file does not exist,
        they have to check its existence by themselves.
        """
        if not os.path.isfile(filename):
            with open(filename, 'w'):
                pass

        with open(filename, 'r+') as file:
            with closing(self.get_conn()) as conn:
                with closing(conn.cursor()) as cur:
                    cur.copy_expert(sql, file)
                    file.truncate(file.tell())
                    conn.commit()

    def bulk_load(self, table, tmp_file):
        """
        Loads a tab-delimited file into a database table
        """
        self.copy_expert("COPY {table} FROM STDIN".format(table=table), tmp_file)

    def bulk_dump(self, table, tmp_file):
        """
        Dumps a database table into a tab-delimited file
        """
        self.copy_expert("COPY {table} TO STDOUT".format(table=table), tmp_file)

    # pylint: disable=signature-differs
    @staticmethod
    def _serialize_cell(cell, conn):
        """
        Postgresql will adapt all arguments to the execute() method internally,
        hence we return cell without any conversion.
        See http://initd.org/psycopg/docs/advanced.html#adapting-new-types for
        more information.
        :param cell: The cell to insert into the table
        :type cell: object
        :param conn: The database connection
        :type conn: connection object
        :return: The cell
        :rtype: object
        """
        return cell

    @staticmethod
    def _generate_insert_sql(table, values, target_fields, replace, **kwargs):
        """
        Static helper method that generate the INSERT SQL statement.
        The REPLACE variant is specific to MySQL syntax.
        :param table: Name of the target table
        :type table: str
        :param values: The row to insert into the table
        :type values: tuple of cell values
        :param target_fields: The names of the columns to fill in the table
        :type target_fields: iterable of strings
        :param replace: Whether to replace instead of insert
        :type replace: bool
        :param replace_index: the column or list of column names to act as
            index for the ON CONFLICT clause
        :type replace_index: str or list
        :return: The generated INSERT or REPLACE SQL statement
        :rtype: str
        """
        placeholders = ["%s", ] * len(values)
        replace_index = kwargs.get("replace_index", None)

        if target_fields:
            target_fields_fragment = ", ".join(target_fields)
            target_fields_fragment = "({})".format(target_fields_fragment)
        else:
            target_fields_fragment = ''

        sql = "INSERT INTO {0} {1} VALUES ({2})".format(
            table,
            target_fields_fragment,
            ",".join(placeholders))

        if replace:
            if target_fields is None:
                raise ValueError("PostgreSQL ON CONFLICT upsert syntax requires column names")
            if replace_index is None:
                raise ValueError("PostgreSQL ON CONFLICT upsert syntax requires an unique index")
            if isinstance(replace_index, str):
                replace_index = [replace_index]
            replace_index_set = set(replace_index)

            replace_target = [
                "{0} = excluded.{0}".format(col)
                for col in target_fields
                if col not in replace_index_set
            ]
            sql += " ON CONFLICT ({0}) DO UPDATE SET {1}".format(
                ", ".join(replace_index),
                ", ".join(replace_target),
            )
        return sql
