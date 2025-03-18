# coding: utf8
# pylint: disable=too-many-arguments
# pylint: disable=invalid-name
# pylint: disable=no-self-use
# pylint: disable=arguments-differ

import re
import logging

import requests

from .tools import dump_request_error, quote_parameters, tsv_dump, to_bytes
from .errors import InternalError, ProgrammingError, OperationalError


SHIELDED_ERROR_CODES = (159,)

threadsafety = 2
paramstyle = 'format'

logger = logging.getLogger('clickhouse_client')


def connect(*args, **kwargs):
    return Connection(*args, **kwargs)


class BaseCursor(object):
    def __init__(self, connection):
        self._connection = connection
        self._response = None

    @property
    def connection(self):
        return self._connection

    def execute(self, query, data=None, **kwargs):
        try:
            self._response = self._connection.request(
                query, data=data, **kwargs
            )
        except requests.exceptions.RequestException as exc:
            if exc.response is not None:
                content = exc.response.content
                exc = OperationalError.from_response(exc.response, orig=exc)
                if exc.code in self._connection.shielded_errors:
                    logger.warning('%s', content)
                else:
                    raise exc
            else:
                raise OperationalError(str(exc), orig=exc)

    def fetchall(self):
        result = self._response.content if self._response else ''
        self._response = None
        return result


class JsonCompactCursor(BaseCursor):
    '''Read and write cursor to ClickHouse data base.'''

    # pylint: disable=inconsistent-return-statements

    def __init__(self, connection):
        super(JsonCompactCursor, self).__init__(connection)
        self._response_content = None
        self._additional_info = None
        self.arraysize = 1

    @property
    def response_content(self):
        if self._response_content is None:
            self._load_data()
        return self._response_content

    def _load_data_resp(self):  # overridable
        return self._response.json()

    def _load_data(self):
        resp_dict = self._load_data_resp()
        self._response_content = resp_dict['data']
        additional_info = dict(resp_dict)
        additional_info.pop('data', None)
        self._additional_info = additional_info

    @property
    def rowcount(self):
        return len(self.response_content) if self.response_content is not None else -1

    @staticmethod
    def _prepare_query(query):
        format_in_query = re.search(r'[)\s]FORMAT\s', query, re.IGNORECASE)
        normilized_query = query.lower().strip()
        select_in_query = normilized_query.startswith(('select', 'show'))
        insert_in_query = normilized_query.startswith('insert')
        if format_in_query and (select_in_query or insert_in_query):
            raise ProgrammingError('Formatting is not available')
        if select_in_query:
            query += ' FORMAT JSONCompact'
        elif insert_in_query:
            query += ' FORMAT TabSeparated'
        return query

    @classmethod
    def mogrify(cls, query, parameters=None, full_prepare=True):
        """
        Return a query string after arguments binding. The string
        returned is exactly the one that would be sent to the database
        running the execute() method or similar.

        The returned string is always a bytes string.

        This is an extension to the DB API 2.0, similar to psycopg2.
        """
        if parameters is not None:
            query_text = query % quote_parameters(parameters)
        else:
            query_text = query
        if full_prepare:
            query_text = cls._prepare_query(query_text)
        return query_text

    def execute(self, query, data=None, parameters=None, **kwargs):
        """Execute query.

        query -- string.

        parameters -- tuple of strings. All parameters are properly escaped and
        inserted into query.
        Example: cur.execute('SELECT * WHERE field = %s', parameters=(
        "текст с /",))
        Note! don't use this feature for query with symbol %.

        kwargs added to params of requests.
        Example: cur.execute(some_query, max_network_bytes=80000000000)"""

        self._response_content = None
        ts_data = b""
        if data is not None:
            ts_data = tsv_dump(data)
        if parameters is not None:
            query_text = query % quote_parameters(parameters)
        else:
            query_text = query
        query_text_with_format = self._prepare_query(query_text)
        super(JsonCompactCursor, self).execute(query_text_with_format, data=ts_data, **kwargs)

    def fetchall(self):
        result = self.response_content
        self._response_content = []
        return result

    def fetchone(self):
        if self.response_content:
            return self.response_content.pop(0)

    def fetchmany(self, size=None):
        size = size or self.arraysize
        result = self.response_content[:size]
        self._response_content = self.response_content[size:]
        return result

    def get_additional_info(self):
        if self._response_content is None:
            self._load_data()
        return self._additional_info


Cursor = JsonCompactCursor


class Connection(object):

    # pylint: disable=too-many-instance-attributes

    def __init__(
            self, host, port, username="", password="",
            connect_timeout=60, read_timeout=60, ssl=False,
            cursor_class=None,
            session_params=None, session_headers=None,
            _password_mode="basic", shielded_errors=None):
        """Connection to ChickHouse.
         session_params add to every request params
        """
        self.host = host
        self.port = port
        self.ssl = ssl
        self.connect_timeout = connect_timeout
        self.read_timeout = read_timeout
        self._closed = False
        self._cursor_class = cursor_class or Cursor
        session = requests.Session()
        self._session = session
        session.params = session_params or {}
        session.headers = session_headers or {}
        self._username = to_bytes(username)
        self._password = to_bytes(password)
        self.shielded_errors = shielded_errors if shielded_errors is not None else SHIELDED_ERROR_CODES
        if username or password:
            self._set_auth(session=session, username=username, password=password, mode=_password_mode)

    def _set_auth(self, session, username, password, mode):
        username = username or ""
        password = password or ""
        if mode == "headers":
            session.headers["X-Clickhouse-User"] = username
            session.headers["X-Clickhouse-Key"] = password
        elif mode == "params":
            session.params['user'] = username
            session.params['password'] = password
        elif mode == "basic":
            from requests.auth import HTTPBasicAuth
            session.auth = HTTPBasicAuth(username, password)
        else:
            raise Exception("Unknown password mode")

    @property
    def username(self):
        return self._username

    @property
    def password(self):
        return self._password

    def request(self, query, data='', **kwargs):
        """kwargs add to request params.
        value from kwargs overrides default (from session params) value"""
        if self._closed:
            raise InternalError('Connection is closed')
        logger.info('%s@%s: %s', self.username, self.host, query)
        if not data:
            params = kwargs
            data = query
        else:
            params = dict(
                kwargs,
                query=query,
            )

        with dump_request_error():
            response = self._session.post(
                self.server_uri,
                # note: all params are these params merged into _session.params
                params=params,
                data=data,
                timeout=(self.connect_timeout, self.read_timeout)
            )
            response.raise_for_status()
            logger.info(
                'status code: %s \nheaders: %s \ncookies: %s \nsize: %d',
                response.status_code,
                response.headers,
                response.cookies.get_dict(),
                len(response.content)
            )
        return response

    @property
    def server_uri(self):
        return '{proto}://{host}:{port}'.format(
            proto='https' if self.ssl else 'http',
            host=self.host, port=self.port)

    def cursor(self):
        if self._closed:
            raise InternalError('Connection is closed')
        return self._cursor_class(self)

    def close(self):
        self._closed = True
        self._session.close()
