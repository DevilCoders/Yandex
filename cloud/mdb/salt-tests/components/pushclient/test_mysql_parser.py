from mysql_error_log_parser import ErrorLogParser
from mysql_general_log_parser import GeneralLogParser
from mysql_slow_query_log_parser import SlowQueryLogParser
from mysql_audit_log_parser import AuditLogParser


def test_error_log_parsing_valid_string_notz():
    key = "1;1;1;"
    raw_record = "2019-01-23T13:41:45.818230Z 0 [Warning] Changed limits: max_open_files: 1024 (requested 5000)"
    log_record = key + raw_record
    parser = ErrorLogParser()
    parser.initialize([])

    expected = {
        'key': key,
        'datetime': '2019-01-23T13:41:45.818230Z',
        'id': '0',
        'status': 'Warning',
        'message': "Changed limits: max_open_files: 1024 (requested 5000)",
        'timestamp': 1548250905,
        'ms': '818',
        'raw': raw_record,
    }

    match_object = parser.parse_record(log_record)
    parser.add_time_data(match_object)
    assert expected == match_object


def test_error_log_parsing_valid_string():
    key = "1;1;1;"
    raw_record = (
        "2018-11-22T21:26:36.090455+03:00 75 [Note] Access denied for user 'UNKNOWN_MYSQL_"
        "USER'@'localhost' (using password: NO)"
    )
    log_record = key + raw_record
    parser = ErrorLogParser()
    parser.initialize([])

    expected = {
        'key': key,
        'datetime': '2018-11-22T21:26:36.090455+03:00',
        'id': '75',
        'status': 'Note',
        'message': "Access denied for user 'UNKNOWN_MYSQL_USER'@'localhost' (using password: NO)",
        'timestamp': 1542911196,
        'ms': '90',
        'raw': raw_record,
    }

    match_object = parser.parse_record(log_record)
    parser.add_time_data(match_object)
    assert expected == match_object


def test_general_log_parsing_valid_string():
    key = "1;1;1;"
    raw = "2018-12-07T13:41:52.285249+03:00	    3 Connect	UNKNOWN_MYSQL_USER@localhost on  using Socket"
    log_record = key + raw
    parser = GeneralLogParser()

    expected = {
        'key': key,
        'datetime': '2018-12-07T13:41:52.285249+03:00',
        'id': '3',
        'command': 'Connect',
        'argument': 'UNKNOWN_MYSQL_USER@localhost on  using Socket',
        'raw': raw,
    }
    match_object = parser.parse_record(log_record)
    assert expected == match_object


def test_general_log_parsing_begining_not_matches():
    log_string = (
        "/usr/sbin/mysqld, Version: 5.7.23-23-57-log (Percona XtraDB Cluster (GPL), Release rel23, Revision"
        " f5578f0, WSREP version 31.31, wsrep_31.31). started with:"
    )
    parser = GeneralLogParser()
    assert parser.parse_record(log_string) is None


def test_general_log_parsing_multiline_queries():
    log_record = "1;1;1;2021-03-17T00:01:01.939152+03:00        55753 Query     SELECT IFNULL(HOST, \"NULL\"), IP\n1;1;1;    FROM performance_schema.host_cache"
    parser = GeneralLogParser()

    expected = {
        'key': '1;1;1;',
        'datetime': '2021-03-17T00:01:01.939152+03:00',
        'id': '55753',
        'command': 'Query',
        'argument': 'SELECT IFNULL(HOST, "NULL"), IP\n    FROM performance_schema.host_cache',
        'raw': "2021-03-17T00:01:01.939152+03:00        55753 Query     SELECT IFNULL(HOST, \"NULL\"), IP\n    FROM performance_schema.host_cache",
    }

    match_object = parser.parse_record(log_record)
    assert expected == match_object


def test_slow_query_log_parsing_valid_record():
    log_record = '''1;1;1;# Time: 2018-12-07T14:57:34.167452+03:00
1;2;1;# User@Host: admin[admin_braces] @ localhost [braces]  Id:   292
1;3;1;# Schema:   Last_errno: 0  Killed: 0
1;4;1;# Query_time: 100.000282  Lock_time: 0.000000  Rows_sent: 1  Rows_examined: 0  Rows_affected: 0
1;5;1;# Bytes_sent: 58
1;6;1;SET timestamp=1544183854;
1;7;1;select sleep(100);'''
    raw_record = '''# Time: 2018-12-07T14:57:34.167452+03:00
# User@Host: admin[admin_braces] @ localhost [braces]  Id:   292
# Schema:   Last_errno: 0  Killed: 0
# Query_time: 100.000282  Lock_time: 0.000000  Rows_sent: 1  Rows_examined: 0  Rows_affected: 0
# Bytes_sent: 58
SET timestamp=1544183854;
select sleep(100);'''

    parser = SlowQueryLogParser()

    expected = {
        'datetime': '2018-12-07T14:57:34.167452+03:00',
        'user': 'admin_braces',
        'hostname': 'localhost',
        'id': '292',
        'schema': '',
        'last_errno': 0,
        'killed': 0,
        'query_time': 100.000282,
        'lock_time': 0.000000,
        'rows_sent': 1,
        'rows_examined': 0,
        'rows_affected': 0,
        'bytes_sent': 58,
        'query': 'SET timestamp=1544183854; select sleep(100);',
        'key': '1;7;1;',
        'raw': raw_record,
    }
    assert expected == parser.parse_record(log_record)


def test_is_record_start_slow_query():
    line = '12312;1;1231211111;# Time: jssdfj'
    parser = SlowQueryLogParser()
    assert parser.is_record_start(line)


def test_audit_log_parsing_valid_string():
    key = '1;1;1;'
    raw_record = (
        r'{"audit_record":{"name":"Audit","record":"1_2019-01-22T09:54:38","timestamp":"2019-01-22T09:'
        r'54:38 UTC","mysql_version":"5.7.23-23-57-log","startup_optionsi":"--basedir=/usr --datadir=/var/li'
        r'b/mysql --plugin-dir=/usr/lib/mysql/plugin --user=mysql --log-error=/var/log/mysql/error.log --ope'
        r'n-files-limit=65535 --pid-file=/var/run/mysqld/mysqld.pid --socket=/var/run/mysqld/mysqld.sock --p'
        r'ort=3306 --wsrep_start_position=00000000-0000-0000-0000-000000000000:-1","os_version":"x86_64-debi'
        r'an-linux-gnu"}}'
    )
    log_record = key + raw_record

    parser = AuditLogParser()
    expected = {
        'name': 'Audit',
        'record': '1_2019-01-22T09:54:38',
        'datetime': '2019-01-22T09:54:38 UTC',
        'mysql_version': '5.7.23-23-57-log',
        'startup_optionsi': '--basedir=/usr --datadir=/var/lib/mysql --plugin-dir=/usr/lib/mysql/plugin --user=mysql '
        '--log-error=/var/log/mysql/error.log --open-files-limit=65535 --pid-file=/var/run/mysqld'
        '/mysqld.pid --socket=/var/run/mysqld/mysqld.sock --port=3306 --wsrep_start_position=0000'
        '0000-0000-0000-0000-000000000000:-1',
        'os_version': 'x86_64-debian-linux-gnu',
        'command_class': '',
        'connection_id': '',
        'connection_type': '',
        'db': '',
        'hostname': '',
        'ip': '',
        'os_login': '',
        'priv_user': '',
        'proxy_user': '',
        'server_id': '',
        'sqltext': '',
        'status': '',
        'status_code': '',
        'user': '',
        'version': '',
        'key': '1;1;1;',
        'raw': raw_record,
    }
    match_object = parser.parse_record(log_record)
    assert expected == match_object
