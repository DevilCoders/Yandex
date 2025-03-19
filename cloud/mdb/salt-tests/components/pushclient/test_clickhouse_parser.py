import io
import socket
from unittest.mock import patch

from clickhouse_server_log_parser import ServerLogParser


def test_parse_log_record():
    log_record = '1;1;1;2019.01.05 00:00:02.376904 [ 112 ] {388ea812-722a-4c39-b7f0-c20aa52e5db0} <Information> HTTPHandler: Done processing query'
    expected = {
        'key': '1;1;1;',
        'datetime': '2019.01.05 00:00:02.376904',
        'thread': '112',
        'query_id': '388ea812-722a-4c39-b7f0-c20aa52e5db0',
        'severity': 'Information',
        'component': 'HTTPHandler',
        'message': 'Done processing query',
        'timestamp': 1546635602,
        'ms': '376',
        'cluster': 'test_cluster',
        'hostname': socket.getfqdn(),
        'origin': 'clickhouse',
        'log_format': 'dbaas_int_log',
    }

    _test_run(log_record, expected)


def test_parse_multiline_log_record():
    log_record = '''1;1;1;2019.04.27 09:49:00.049645 [ 57 ] {32f77087-7155-4af7-8904-5df14f63d183} <Debug> executeQuery: Query pipeline:
1;2;10;Expression
1;3;20; Expression
1;4;30;  One
1;5;40;'''
    expected = {
        'key': '1;5;40;',
        'datetime': '2019.04.27 09:49:00.049645',
        'thread': '57',
        'query_id': '32f77087-7155-4af7-8904-5df14f63d183',
        'severity': 'Debug',
        'component': 'executeQuery',
        'message': r'Query pipeline:\nExpression\n Expression\n  One\n',
        'timestamp': 1556347740,
        'ms': '49',
        'cluster': 'test_cluster',
        'hostname': socket.getfqdn(),
        'origin': 'clickhouse',
        'log_format': 'dbaas_int_log',
    }

    _test_run(log_record, expected)


def test_parse_log_record_with_function_as_component():
    log_record = (
        r'1;1;1;2019.01.05 00:00:02.376904 [ 112 ] {} <Trace> RPTVdbzDEYPoweAbaRWkjLAxiNGdzpIE.__'
        r'conn_preview_ppp606qbiev2n (StorageReplicatedMergeTree::mergeSelectingTask): Execution took: 280 ms.'
    )
    expected = {
        'key': '1;1;1;',
        'datetime': '2019.01.05 00:00:02.376904',
        'thread': '112',
        'severity': 'Trace',
        'component': 'RPTVdbzDEYPoweAbaRWkjLAxiNGdzpIE.__conn_preview_ppp606qbiev2n '
        '(StorageReplicatedMergeTree::mergeSelectingTask)',
        'message': 'Execution took: 280 ms.',
        'timestamp': 1546635602,
        'ms': '376',
        'cluster': 'test_cluster',
        'hostname': socket.getfqdn(),
        'origin': 'clickhouse',
        'log_format': 'dbaas_int_log',
    }

    _test_run(log_record, expected)


def test_dumps_log_record_with_control_symbols():
    log_record = (
        '1;1;1;2019.01.05 00:00:02.376904 [ 1 ] {} <Information> : '
        'Code: 62, e.displayText() = DB::Exception: Syntax error: failed at position 1 (line 1, col 1): "\t\r"'
    )
    expected = (
        '1;1;1;tskv\tdatetime=2019.01.05 00:00:02.376904\tthread=1\tseverity=Information\tcomponent=\tmessage='
        'Code: 62, e.displayText() = DB::Exception: Syntax error: failed at position 1 (line 1, col 1): "\\t\\r"'
        '\ttimestamp=1546635602\tms=376'
        '\tcluster=test_cluster\thostname=' + socket.getfqdn() + '\torigin=clickhouse\tlog_format=dbaas_int_log'
    )

    _test_run(log_record, expected)


def test_parse_log_with_empty_component():
    log_record = '1;1;1;2019.01.05 00:00:02.376904 [ 1 ] {} <Information> : Starting ClickHouse 19.11 with revision 0'
    expected = {
        'key': '1;1;1;',
        'datetime': '2019.01.05 00:00:02.376904',
        'thread': '1',
        'severity': 'Information',
        'component': '',
        'message': 'Starting ClickHouse 19.11 with revision 0',
        'timestamp': 1546635602,
        'ms': '376',
        'cluster': 'test_cluster',
        'hostname': socket.getfqdn(),
        'origin': 'clickhouse',
        'log_format': 'dbaas_int_log',
    }

    _test_run(log_record, expected)


def _test_run(raw_log_record, expected):
    parser = ServerLogParser()

    with patch('sys.stdout', new_callable=io.StringIO) as mock_stdout:
        parser.run(args='-t Europe/Moscow -c test_cluster'.split(), stream=io.StringIO(raw_log_record))
    printed_result = mock_stdout.getvalue()

    if isinstance(expected, dict):
        expected = expected.pop('key') + 'tskv\t' + '\t'.join('{}={}'.format(k, v) for k, v in expected.items())

    assert printed_result == expected + '\n'
