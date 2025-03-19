import io
import socket
from unittest.mock import patch

from mongodb_log_parser import MongoDBLogParser


def test_error_log_parsing_valid_string():
    log_record = (
        '1;1;1;2019-01-05T00:00:02.271+0300 I CONTROL  [signalProcessingThread] pid=45298 port=27018 64-bit '
        'host=vla-nj72zmkrcu7zbg0i.db.yandex.net'
    )
    expected = {
        'key': '1;1;1;',
        'datetime': '2019-01-05T00:00:02.271+0300',
        'severity': 'I',
        'component': 'CONTROL',
        'context': 'signalProcessingThread',
        'message': r'pid=45298 port=27018 64-bit host=vla-nj72zmkrcu7zbg0i.db.yandex.net',
        'timestamp': 1546635602,
        'ms': '270',
        'cluster': 'test_cluster',
        'hostname': socket.getfqdn(),
        'origin': 'test_origin',
        'log_format': 'dbaas_int_log',
    }
    parser = MongoDBLogParser()

    with patch('sys.stdout', new_callable=io.StringIO) as mock_stdout:
        parser.run(args='-c test_cluster -o test_origin'.split(), stream=io.StringIO(log_record))
    printed_result = mock_stdout.getvalue()

    if isinstance(expected, dict):
        expected = expected.pop('key') + 'tskv\t' + '\t'.join('{}={}'.format(k, v) for k, v in expected.items())

    assert printed_result == expected + '\n'
