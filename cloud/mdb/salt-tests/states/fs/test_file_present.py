# coding: utf-8

from __future__ import print_function, unicode_literals

from cloud.mdb.internal.python.pytest.utils import parametrize
from cloud.mdb.salt.salt._states import fs

from mock import MagicMock, call


def test_file_present_with_yaml_format():
    fs.__states__['file.managed'] = MagicMock()

    fs.file_present(name='test_file', contents={'key': 'value'}, contents_format='yaml')

    assert fs.__states__['file.managed'].call_args_list == [call(name='test_file', contents='key: value\n')]


name = '/var/lib/clickhouse/format_schemas/test_object.capnp'
data = 'test object contents'
cache_path = 'object_cache/file'
url = 'https://bucket.s3-url.y-t.ru/path/to/file'


@parametrize(
    {
        'id': 'download from cache',
        'args': {
            'exists': True,
            'states': {
                'file.managed': [call(name=name, contents=data)],
            },
            'calls': {
                'mdb_s3.object_cached': [],
            },
            'opts': {'test': False},
        },
    },
    {
        'id': 'download by url',
        'args': {
            'exists': False,
            'states': {
                'file.managed': [call(name=name, contents=data)],
            },
            'calls': {'mdb_s3.create_object': [call(None, cache_path, data, s3_bucket=None)]},
            'opts': {'test': False},
        },
    },
    {
        'id': 'download by url(test)',
        'args': {
            'exists': False,
            'states': {
                'file.managed': [call(name=name, contents=data)],
            },
            'calls': {},
            'opts': {'test': True},
        },
    },
)
def test_file_present_with_cache(exists, states, calls, opts):
    def object_exists(_s3_client, key, s3_bucket=None):
        assert key == cache_path
        return exists

    def get_object(_s3_client, key, s3_bucket=None):
        assert key == cache_path
        return data

    def download_object(_url, _auth):
        assert not exists, 'Object should be downloaded from cache'
        assert url == _url
        assert _auth
        return data

    fs.__opts__ = opts
    fs.__salt__['mdb_s3.client'] = lambda: None
    fs.__salt__['mdb_s3.object_exists'] = object_exists
    fs.__salt__['mdb_s3.get_object'] = get_object
    fs.__salt__['fs.download_object'] = download_object

    for function_name in calls.keys():
        fs.__salt__[function_name] = MagicMock()
    for state_name in states.keys():
        fs.__states__[state_name] = MagicMock()

    fs.file_present(name=name, s3_cache_path=cache_path, url=url, use_service_account_authorization=True)

    for function_name, call_list in calls.items():
        assert fs.__salt__[function_name].call_args_list == call_list
    for state_name, call_list in states.items():
        assert fs.__states__[state_name].call_args_list == call_list
