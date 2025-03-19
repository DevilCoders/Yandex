"""
Test for Hadoop job log handler
"""

import datetime
from collections import namedtuple

from dbaas_internal_api.modules.hadoop.jobs import get_api_response
from dbaas_internal_api.utils.logging_read_service import GrpcResponse


LoggingReadResponse = namedtuple('LoggingReadResponse', ['entries', 'next_page_token'])


class Timestamp:
    def __init__(self, time=None):
        self.time = time

    def ToJsonString(self):
        return str(self.time)


class Entry:
    def __init__(self, timestamp=None, message=None):
        self.timestamp = timestamp
        self.message = message


def test_get_api_response():
    job = {'status': 'RUNNING'}
    grpc_response = GrpcResponse(
        data=LoggingReadResponse(
            entries=[
                Entry(timestamp=Timestamp(time=datetime.datetime.now()), message='123'),
                Entry(timestamp=Timestamp(time=datetime.datetime.now()), message='Data Proc job finished.'),
            ],
            next_page_token='456',
        ),
        meta=None,
    )
    api_response = get_api_response(job, grpc_response)
    assert not api_response['next_page_token']
    assert len(api_response['content'].splitlines()) == 2
