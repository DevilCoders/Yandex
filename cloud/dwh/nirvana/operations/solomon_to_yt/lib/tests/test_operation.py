from collections import defaultdict
from datetime import datetime
from unittest import mock

import pytest

from cloud.dwh.clients.solomon import SolomonApiClient
from cloud.dwh.nirvana.operations.solomon_to_yt.lib import exceptions
from cloud.dwh.nirvana.operations.solomon_to_yt.lib import operation
from cloud.dwh.nirvana.operations.solomon_to_yt.lib.types import YtSplitIntervals
from cloud.dwh.utils.coroutines import async_generator_to_list
from cloud.dwh.utils.datetimeutils import MSK_TIMEZONE_OBJ


@pytest.fixture(autouse=True)
def mock_update_progressbar():
    with mock.patch('cloud.dwh.nirvana.operations.solomon_to_yt.lib.operation._update_progressbar'):
        yield


@pytest.mark.parametrize('selectors, labels, expected', [
    ('', [], '{}'),
    ('{}', [], '{}'),
    ('{}', [operation.Label('a', '1')], '{a="1"}'),
    ('{}', [operation.Label('a', '1'), operation.Label('b', '2')], '{a="1",b="2"}'),
    ('', [operation.Label('a', '1')], '{a="1"}'),
    ('', [operation.Label('a', '1'), operation.Label('b', '2')], '{a="1",b="2"}'),
    ('c="1"', [], '{c="1"}'),
    ('{c="1"}', [], '{c="1"}'),
    ('{c="1"}', [operation.Label('a', '1')], '{c="1",a="1"}'),
    ('{c="1"}', [operation.Label('a', '1'), operation.Label('b', '2')], '{c="1",a="1",b="2"}'),
    ('c="1", d="2"', [], '{c="1", d="2"}'),
    ('{c="1", d="2"}', [], '{c="1", d="2"}'),
    ('{c="1", d="2"}', [operation.Label('a', '1')], '{c="1", d="2",a="1"}'),
    ('{c="1", d="2"}', [operation.Label('a', '1'), operation.Label('b', '2')], '{c="1", d="2",a="1",b="2"}'),
])
def test_add_labels_to_selectors(selectors, labels, expected):
    result = operation._add_labels_to_selectors(selectors, labels)
    assert result == expected


@pytest.mark.parametrize('selectors, partition_labels, expected', [
    ('', [], ['']),
    ('', [[operation.Label("b", "1")]], ['{b="1"}']),
    ('', [[operation.Label("b", "1"), operation.Label("b", "2")], [operation.Label("c", "3")], ], ['{b="1",c="3"}', '{b="2",c="3"}']),
    ('', [[operation.Label("b", "1"), operation.Label("b", "2")], [operation.Label("c", "3"), operation.Label("c", "4")], ], ['{b="1",c="3"}', '{b="1",c="4"}', '{b="2",c="3"}', '{b="2",c="4"}']),
    ('{a="1"}', [], ['{a="1"}']),
    ('{a="1"}', [[operation.Label("b", "1")]], ['{a="1",b="1"}']),
    ('{a="1"}', [[operation.Label("b", "1"), operation.Label("b", "2")], [operation.Label("c", "3")], ], ['{a="1",b="1",c="3"}', '{a="1",b="2",c="3"}']),
    ('{a="1"}', [[operation.Label("b", "1"), operation.Label("b", "2")], [operation.Label("c", "3"), operation.Label("c", "4")], ],
     ['{a="1",b="1",c="3"}', '{a="1",b="1",c="4"}', '{a="1",b="2",c="3"}', '{a="1",b="2",c="4"}']),
])
def test_iterate_with_labels(selectors, partition_labels, expected):
    result = operation._iterate_with_labels(selectors, partition_labels)
    assert list(result) == expected


@pytest.mark.asyncio
async def test_read_solomon_data_with_partition_labels():
    solomon_client = mock.AsyncMock()
    project = 'dwh'
    selectors = '{a="1"}'
    expression = 'drop_nan({selectors})'
    from_dttm = MSK_TIMEZONE_OBJ.localize(datetime(2020, 2, 1, 0, 0, 0))
    to_dttm = MSK_TIMEZONE_OBJ.localize(datetime(2020, 2, 1, 1, 0, 0))
    partition_labels = [[operation.Label('b', '1'), operation.Label('b', '2')], [operation.Label('c', '3')]]

    solomon_client.data.side_effect = [
        {
            'vector': [
                {
                    'timeseries': {
                        'kind': 'KIND',
                        'type': 'TYPE',
                        'labels': {'a': '1', 'b': '1', 'c': '3'},
                        'timestamps': [1, 2],
                        'values': [.3, .4],
                    }
                }
            ]
        },
        {
            'vector': [
                {
                    'timeseries': {
                        'kind': 'KIND',
                        'type': 'TYPE',
                        'labels': {'a': '1', 'b': '2', 'c': '3'},
                        'timestamps': [5],
                        'values': [.6],
                    }
                }
            ]
        },
    ]

    result = await async_generator_to_list(
        operation._read_solomon_data(
            solomon_client=solomon_client,
            project=project,
            selectors=selectors,
            expression=expression,
            from_dttm=from_dttm,
            to_dttm=to_dttm,
            partition_labels=partition_labels,
        ),
    )

    solomon_client.data.assert_has_calls([
        mock.call(
            project=project,
            body={
                'from': 1580504400000,
                'program': 'drop_nan({a="1",b="1",c="3"})',
                'to': 1580508000000,
            },
        ),
        mock.call(
            project=project,
            body={
                'from': 1580504400000,
                'program': 'drop_nan({a="1",b="2",c="3"})',
                'to': 1580508000000,
            },
        ),
    ])

    expected = [
        {
            '_meta': {'kind': 'KIND', 'type': 'TYPE'},
            'a': '1',
            'b': '1',
            'c': '3',
            'timestamp': 1,
            'value': '0.3',
        },
        {
            '_meta': {'kind': 'KIND', 'type': 'TYPE'},
            'a': '1',
            'b': '1',
            'c': '3',
            'timestamp': 2,
            'value': '0.4',
        },
        {
            '_meta': {'kind': 'KIND', 'type': 'TYPE'},
            'a': '1',
            'b': '2',
            'c': '3',
            'timestamp': 5,
            'value': '0.6',
        },
    ]

    assert result == expected


@pytest.mark.asyncio
async def test_read_solomon_data():
    solomon_client = mock.AsyncMock()
    project = 'dwh'
    selectors = '{a="1"}'
    expression = 'drop_nan({selectors})'
    from_dttm = MSK_TIMEZONE_OBJ.localize(datetime(2020, 2, 1, 0, 0, 0))
    to_dttm = MSK_TIMEZONE_OBJ.localize(datetime(2020, 2, 1, 1, 0, 0))
    partition_labels = []

    solomon_client.data.side_effect = [
        {
            'vector': [
                {
                    'timeseries': {
                        'kind': 'KIND',
                        'type': 'TYPE',
                        'labels': {'a': '1', 'b': '2'},
                        'timestamps': [1, 2],
                        'values': [.3, .4],
                    }
                }
            ]
        },
    ]

    result = await async_generator_to_list(
        operation._read_solomon_data(
            solomon_client=solomon_client,
            project=project,
            selectors=selectors,
            expression=expression,
            from_dttm=from_dttm,
            to_dttm=to_dttm,
            partition_labels=partition_labels,
        ),
    )

    solomon_client.data.assert_called_once_with(
        project=project,
        body={
            'from': 1580504400000,
            'program': 'drop_nan({a="1"})',
            'to': 1580508000000,
        }
    )

    expected = [
        {
            '_meta': {'kind': 'KIND', 'type': 'TYPE'},
            'a': '1',
            'b': '2',
            'timestamp': 1,
            'value': '0.3',
        },
        {
            '_meta': {'kind': 'KIND', 'type': 'TYPE'},
            'a': '1',
            'b': '2',
            'timestamp': 2,
            'value': '0.4',
        },
    ]

    assert result == expected


@pytest.mark.parametrize('path, list_result, expected', [
    ('//tmp', [], None),
    ('//tmp', ['//tmp/1d/table1'], 'table1'),
    ('//tmp', ['//tmp/1d/table1', '//tmp/1d/table2'], 'table2'),
    ('//tmp', ['//tmp/1d/table2', '//tmp/1d/table1'], 'table2'),
])
@mock.patch('cloud.dwh.nirvana.operations.solomon_to_yt.lib.operation.yt', autospec=True)
def test_maybe_get_last_yt_table(mock_yt, path, list_result, expected):
    mock_yt.list.return_value = list_result

    result = operation._maybe_get_last_yt_table(path)

    mock_yt.list.assert_called_once_with(path)

    assert result == expected


@pytest.mark.parametrize('yt_list_result, yt_folder, yt_split_interval, default_dttm, expected', [
    ([], '//tmp/1d', YtSplitIntervals.Types.DAILY, datetime(2020, 12, 31, 0, 0, 0), datetime(2020, 12, 31, 0, 0, 0)),
    (['2020-01-04T01:00:00'], '//tmp/1d', YtSplitIntervals.Types.HOURLY, datetime(2020, 12, 31, 0, 0, 0), MSK_TIMEZONE_OBJ.localize(datetime(2020, 1, 4, 2, 0, 0))),
    (['2020-01-04'], '//tmp/1d', YtSplitIntervals.Types.DAILY, datetime(2020, 12, 31, 0, 0, 0), MSK_TIMEZONE_OBJ.localize(datetime(2020, 1, 5, 0, 0, 0))),
    (['2020-01-01'], '//tmp/1d', YtSplitIntervals.Types.MONTHLY, datetime(2020, 12, 31, 0, 0, 0), MSK_TIMEZONE_OBJ.localize(datetime(2020, 2, 1, 0, 0, 0))),
])
@mock.patch('cloud.dwh.nirvana.operations.solomon_to_yt.lib.operation.yt', autospec=True)
def test_get_from_dttm_via_yt_table(mock_yt, yt_list_result, yt_folder, yt_split_interval, default_dttm, expected):
    mock_yt.list.return_value = yt_list_result

    result = operation._get_from_dttm_via_yt_table(yt_folder=yt_folder, yt_split_interval=yt_split_interval, default_dttm=default_dttm)

    mock_yt.list.assert_called_once_with(yt_folder)

    assert result == expected


@pytest.mark.asyncio
async def test_get_partition_labels_empty_input_label_names():
    solomon_client = mock.AsyncMock()

    result = await operation._get_partition_labels(
        solomon_client=solomon_client,
        project='dwh',
        label_names=[],
        selectors='',
        limit=100,
    )

    solomon_client.get_labels.assert_not_called()

    assert result == []


@pytest.mark.parametrize('label_names, exist_label', [
    (['a'], 'label1'),
    (['a', 'label1'], 'label1'),
])
@pytest.mark.asyncio
async def test_get_partition_labels_not_found_labels(label_names, exist_label):
    solomon_client = mock.AsyncMock()

    solomon_client.get_labels.return_value = {
        'labels': [
            {
                'name': exist_label,
                'values': ['v1', 'v2'],
                'truncated': False,
            }
        ],
    }

    with pytest.raises(exceptions.LabelNotFoundError):
        await operation._get_partition_labels(
            solomon_client=solomon_client,
            project='dwh',
            label_names=label_names,
            selectors='',
            limit=100,
        )

    solomon_client.get_labels.assert_called_once_with(project='dwh', params={'names': label_names, 'limit': 100, 'selectors': ''})


@pytest.mark.asyncio
async def test_get_partition_labels_truncated_response():
    solomon_client = mock.AsyncMock()

    solomon_client.get_labels.return_value = {
        'labels': [
            {
                'name': 'a',
                'values': ['v1', 'v2'],
                'truncated': True,
            }
        ],
    }

    with pytest.raises(exceptions.TruncatedResponseError):
        await operation._get_partition_labels(
            solomon_client=solomon_client,
            project='dwh',
            label_names=['a'],
            selectors='',
            limit=100,
        )

    solomon_client.get_labels.assert_called_once_with(project='dwh', params={'names': ['a'], 'limit': 100, 'selectors': ''})


@pytest.mark.parametrize('project, label_names, selectors, limit, expected', [
    ('dwh', ['a'], '{c=1}', 1, [[operation.Label('a', '1'), operation.Label('a', '2')]]),
    ('dwh', ['a'], '', 100, [[operation.Label('a', '1'), operation.Label('a', '2')]]),
    ('dwh', ['b'], '', 100, [[operation.Label('b', '3')]]),
    ('dwh', ['a', 'b'], '', 100, [[operation.Label('a', '1'), operation.Label('a', '2')], [operation.Label('b', '3')]]),
])
@pytest.mark.asyncio
async def test_get_partition_labels(project, label_names, selectors, limit, expected):
    solomon_client = mock.AsyncMock()

    solomon_client.get_labels.return_value = {
        'labels': [
            {
                'name': 'a',
                'values': ['1', '2'],
                'truncated': False,
            },
            {
                'name': 'b',
                'values': ['3'],
                'truncated': False,
            }
        ],
    }

    result = await operation._get_partition_labels(
        solomon_client=solomon_client,
        project=project,
        label_names=label_names,
        selectors=selectors,
        limit=limit,
    )

    solomon_client.get_labels.assert_called_once_with(project=project, params={'names': label_names, 'limit': limit, 'selectors': selectors})

    assert result == expected


@mock.patch.object(SolomonApiClient, 'data', autospec=True)
@mock.patch.object(SolomonApiClient, 'get_labels', autospec=True)
@mock.patch('cloud.dwh.nirvana.operations.solomon_to_yt.lib.operation.get_now_msk', autospec=True)
@mock.patch('cloud.dwh.nirvana.operations.solomon_to_yt.lib.operation.yt', autospec=True)
@mock.patch('cloud.dwh.nirvana.operations.solomon_to_yt.lib.operation.nv', autospec=True)
@pytest.mark.asyncio
async def test_run_operation(mock_nv, mock_yt, mock_get_now_msk, mock_get_labels, mock_data):
    job_context = mock_nv.context.return_value
    mr_inputs = job_context.get_mr_inputs.return_value
    job_context.get_mr_cluster.return_value.get_name.return_value = 'hahn'
    mr_inputs.get.return_value.get_path.return_value = '//test/path'

    job_context.get_parameters.return_value = {
        'yt-tables-split-interval': YtSplitIntervals.Types.HOURLY,
        'yt-token': 'token-yt',
        'yt-table-ttl': 1000,
        'solomon-endpoint': 'example.com',
        'solomon-token': 'token-solomon',
        'solomon-project': 'test-proj',
        'solomon-selectors': '{a="1"}',
        'solomon-expression': 'drop_nan({selectors})',
        'solomon-default-from-dttm': '2010-12-31T10:45:34+0000',
        'solomon-partition-by-labels': ['label1'],
        'solomon-page-size': 3600,
        'solomon-now-lag': 600,
    }

    mock_get_now_msk.return_value = MSK_TIMEZONE_OBJ.localize(datetime(2020, 12, 31, 13, 10, 00))

    mock_yt.config = defaultdict(dict)
    mock_yt.list.return_value = ['//test/path/1h/2020-12-31T10:00:00', '//test/path/1h/2020-12-31T11:00:00']
    mock_yt.exists.return_value = False

    mock_get_labels.return_value = {
        'labels': [
            {
                'name': 'label1',
                'values': ['v1', 'v2'],
                'truncated': False,
            }
        ],
    }

    mock_data.side_effect = [
        {
            'vector': [
                {
                    'timeseries': {
                        'kind': 'KIND',
                        'type': 'TYPE',
                        'labels': {'x': '1', 'y': '2'},
                        'timestamps': [1, 2],
                        'values': [.3, .4],
                    }
                }
            ]
        },
        {
            'vector': [
                {
                    'timeseries': {
                        'kind': 'KIND',
                        'type': 'TYPE',
                        'labels': {'x': '3', 'z': '4'},
                        'timestamps': [5],
                        'values': [.6],
                    },
                },
            ],
        },
    ]

    await operation.run_operation()

    assert mock_yt.config == {'token': 'token-yt', 'proxy': {'url': 'hahn'}, 'write_parallel': {'enable': True, 'max_thread_count': 100, 'unordered': True}}
    mock_get_labels.assert_called_once_with(mock.ANY, project='test-proj', params={'names': ['label1'], 'limit': 10000, 'selectors': '{a="1"}'})
    mock_yt.list.assert_called_once_with('//test/path/1h')
    mock_yt.exists.assert_called_once_with('//test/path/1h')
    mock_yt.create.assert_called_once_with('map_node', '//test/path/1h', recursive=True)
    mock_data.assert_has_calls([
        mock.call(mock.ANY, project='test-proj', body={'from': 1609405200000, 'program': 'drop_nan({a="1",label1="v1"})', 'to': 1609408799000}),
        mock.call(mock.ANY, project='test-proj', body={'from': 1609405200000, 'program': 'drop_nan({a="1",label1="v2"})', 'to': 1609408799000}),
    ], any_order=True)
    mock_yt.TablePath.assert_has_calls([
        mock.call('//test/path/1h/2020-12-31T12:00:00', append=True)
    ])
    mock_yt.write_table.assert_called_once_with(
        table=mock_yt.TablePath.return_value,
        format=mock_yt.JsonFormat.return_value,
        input_stream=[
            {'x': '1', 'y': '2', '_meta': {'kind': 'KIND', 'type': 'TYPE'}, 'timestamp': 1, 'value': '0.3'},
            {'x': '1', 'y': '2', '_meta': {'kind': 'KIND', 'type': 'TYPE'}, 'timestamp': 2, 'value': '0.4'},
            {'x': '3', 'z': '4', '_meta': {'kind': 'KIND', 'type': 'TYPE'}, 'timestamp': 5, 'value': '0.6'},
        ],
    )
    mock_yt.set.assert_called_once_with('//test/path/1h/2020-12-31T12:00:00/@expiration_timeout', 1000)
