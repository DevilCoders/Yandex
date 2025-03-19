import io
import json
import socket
from unittest.mock import patch

from log_parser import LogParser

# language=json - for intellij ide
PRETTY_LOG_RECORD = """
{
    "atype": "clientMetadata",
    "ts": {
        "$date": "2022-03-18T00:00:37.021+03:00"
    },
    "uuid": {
        "$binary": "dQpRygCGSeGPGgMkxOwqPg==",
        "$type": "04"
    },
    "local": {
        "ip": "10.129.0.10",
        "port": 27018
    },
    "remote": {
        "ip": "10.129.0.10",
        "port": 40398
    },
    "users": [],
    "roles": [],
    "param": {
        "localEndpoint": {
            "ip": "10.129.0.10",
            "port": 27018
        },
        "clientMetadata": {
            "driver": {
                "name": "PyMongo",
                "version": "3.7.2"
            },
            "os": {
                "type": "Linux",
                "name": "Linux",
                "architecture": "x86_64",
                "version": "4.19.114-29"
            },
            "platform": "CPython 3.6.9.final.0",
            "application": {
                "name": "mdb-metrics_is-primary"
            }
        }
    },
    "result": 0
}
"""
RAW_LOG_RECORD = json.dumps(json.loads(PRETTY_LOG_RECORD))  # compact represent in single line


def test_parse_mongodb_audit_log():
    test_line = 'x;y;z;' + RAW_LOG_RECORD
    parser = LogParser()

    with patch('sys.stdout', new_callable=io.StringIO) as mock_stdout:
        parser.run(
            args='-c test_cluster -o test_origin -i json -f json -dts.$date'.split(), stream=io.StringIO(test_line)
        )
    printed_result = mock_stdout.getvalue()

    assert printed_result == (
        'x;y;z;'
        + json.dumps(
            {
                'datetime': '2022-03-18T00:00:37.021+03:00',
                'message': RAW_LOG_RECORD,
                'timestamp': 1647550837,
                'ms': '20',
                'cluster': 'test_cluster',
                'hostname': socket.getfqdn(),
                'origin': 'test_origin',
                'log_format': 'dbaas_int_log',
            }
        )
        + '\n'
    )
