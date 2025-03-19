import asyncio
import dataclasses
import datetime
import itertools
import json
import logging
import re
from pathlib import Path
from typing import AsyncGenerator
from typing import Generator
from typing import List
from typing import Optional
from typing import Tuple

import nirvana.mr_job_context as nv
import tenacity
import yt.wrapper as yt

from cloud.dwh.clients.solomon import SolomonApiClient
from cloud.dwh.nirvana.operations.solomon_to_yt.lib.exceptions import LabelNotFoundError
from cloud.dwh.nirvana.operations.solomon_to_yt.lib.exceptions import TruncatedResponseError
from cloud.dwh.nirvana.operations.solomon_to_yt.lib.types import YtSplitIntervals
from cloud.dwh.utils.coroutines import async_generator_to_list
from cloud.dwh.utils.coroutines import parallel_limiter
from cloud.dwh.utils.datetimeutils import MSK_TIMEZONE_OBJ
from cloud.dwh.utils.datetimeutils import dttm_paginator
from cloud.dwh.utils.datetimeutils import get_now_msk

LOG = logging.getLogger(__name__)

SOLOMON_BATCH_SIZE = 500
LABELS_LIMIT = 10000
SELECTORS_RE = re.compile(r'^{?(?P<selectors>.*?)}?$')
ONE_SECOND = datetime.timedelta(seconds=1)

retry = tenacity.retry(
    stop=tenacity.stop_after_attempt(5),
    wait=tenacity.wait_random(0, 3),
    reraise=True,
)


@dataclasses.dataclass
class Label:
    name: str
    value: str


PartitionLabelsList = List[List[Label]]


def _update_progressbar(message: str, value: float):
    job_context = nv.context()
    with open(job_context.get_status().get_log(), 'a') as file:
        progressbar_msg = json.dumps({'message': message, 'progress': value})
        LOG.debug('Update progressbar: %s', progressbar_msg)

        file.write(progressbar_msg + '\n')


def _add_labels_to_selectors(selectors: str, labels: Tuple[Label, ...]) -> str:
    selectors = re.match(SELECTORS_RE, selectors).group('selectors')
    labels_selectors = f'''{",".join(f'{label.name}="{label.value}"' for label in labels)}'''

    return f'{{{",".join((selectors, labels_selectors)).strip(",")}}}'


def _iterate_with_labels(selectors: str, partition_labels: PartitionLabelsList) -> Generator[str, None, None]:
    if not partition_labels:
        yield selectors
        return

    for labels in itertools.product(*partition_labels):  # type: Tuple[Label, ...]
        yield _add_labels_to_selectors(selectors=selectors, labels=labels)


async def _read_solomon_data(solomon_client: SolomonApiClient,
                             project: str,
                             selectors: str,
                             expression: str,
                             from_dttm: datetime.datetime,
                             to_dttm: datetime.datetime,
                             partition_labels: PartitionLabelsList) -> AsyncGenerator[dict, None]:
    coros = []
    get_data_func = parallel_limiter(SOLOMON_BATCH_SIZE)(retry(solomon_client.data))

    for s in _iterate_with_labels(selectors=selectors, partition_labels=partition_labels):
        params = {
            'from': int(from_dttm.timestamp() * 1000),
            'program': expression.format(selectors=s),
            'to': int(to_dttm.timestamp() * 1000),
        }

        coros.append(get_data_func(project=project, body=params))

    for coro in asyncio.as_completed(coros):
        result = await coro

        for sensor in result['vector']:
            if 'timeseries' not in sensor:
                continue

            sensor = sensor['timeseries']

            if not sensor.get('values'):
                continue

            if 'value' in sensor['labels'] or 'timestamp' in sensor['labels']:
                raise KeyError(f'There are system fields in solomon data {sensor["labels"]}')

            for timestamp, value in zip(sensor['timestamps'], sensor['values']):
                data = sensor['labels'].copy()
                data['_meta'] = {
                    'kind': sensor.get('kind'),
                    'type': sensor.get('type'),
                }

                data['timestamp'] = timestamp
                data['value'] = str(value)

                yield data


def _maybe_get_last_yt_table(path: str) -> Optional[str]:
    if tables := yt.list(path):
        return Path(sorted(tables)[-1]).name
    else:
        return None


def _get_from_dttm_via_yt_table(yt_folder: str, yt_split_interval: str, default_dttm: datetime.datetime) -> datetime.datetime:
    last_table = _maybe_get_last_yt_table(yt_folder)

    if last_table:
        from_dttm = MSK_TIMEZONE_OBJ.localize(datetime.datetime.fromisoformat(last_table))
        return YtSplitIntervals.ceil_to_interval(dttm=from_dttm, interval=yt_split_interval) + ONE_SECOND
    else:
        return default_dttm


async def _get_partition_labels(solomon_client: SolomonApiClient,
                                project: str,
                                label_names: List[str],
                                selectors: str = '',
                                limit: int = LABELS_LIMIT) -> PartitionLabelsList:
    if not label_names:
        return []

    params = {'names': label_names, 'limit': limit, 'selectors': selectors}
    result = await solomon_client.get_labels(project=project, params=params)

    labels_data = {}

    for label_info in result['labels']:
        if label_info['truncated']:
            raise TruncatedResponseError(f'Label {label_info["name"]} is truncated for limit {limit}')

        if label_info['name'] not in label_names:
            LOG.warning('Unexpected label %s. Expected one of %s', label_info['name'], label_names)
            continue

        labels_data[label_info['name']] = [Label(name=label_info['name'], value=v) for v in label_info['values']]

    if set(label_names) - set(labels_data):
        raise LabelNotFoundError(f'Label(s) not found: {set(label_names) - set(labels_data)}')

    return list(labels_data.values())


async def _upload(solomon_project: str,
                  solomon_selectors: str,
                  solomon_expression: str,
                  yt_destination_folder: str,
                  yt_tables_split_interval: str,
                  yt_table_ttl: int,
                  solomon_default_from_dttm: datetime.datetime,
                  solomon_now_lag: int,
                  solomon_partition_by_label: List[str],
                  solomon_page_size: int,
                  solomon_client: SolomonApiClient):
    partition_labels = await _get_partition_labels(
        solomon_client=solomon_client,
        project=solomon_project,
        selectors=solomon_selectors,
        label_names=solomon_partition_by_label,
    )
    LOG.info('Got partition labels: %s', partition_labels)

    yt_destination_folder = f'{yt_destination_folder}/{YtSplitIntervals.get_interval_short_name(yt_tables_split_interval)}'

    if not yt.exists(yt_destination_folder):
        yt.create('map_node', yt_destination_folder, recursive=True)

    from_dttm = _get_from_dttm_via_yt_table(yt_folder=yt_destination_folder, yt_split_interval=yt_tables_split_interval, default_dttm=solomon_default_from_dttm)

    max_dttm = get_now_msk() - datetime.timedelta(seconds=solomon_now_lag)

    LOG.info('Uploading solomon data from %s to %s', from_dttm.isoformat(), max_dttm.isoformat())

    transaction_attributes = {'title': f'Nirvana Operation: {nv.context().get_meta().get_block_url()}'}
    to_dttm = YtSplitIntervals.ceil_to_interval(dttm=from_dttm, interval=yt_tables_split_interval)
    total_seconds = (max_dttm - from_dttm).total_seconds()
    while to_dttm <= max_dttm:
        table_name = f'{yt_destination_folder}/{YtSplitIntervals.format_by_interval(to_dttm, yt_tables_split_interval)}'

        LOG.info('Uploading to table %s from %s to %s', table_name, from_dttm.isoformat(), to_dttm.isoformat())

        with yt.Transaction(attributes=transaction_attributes):
            for batch_from_dttm, batch_to_dttm in dttm_paginator(from_dttm, to_dttm, page_size=solomon_page_size):
                LOG.debug('Processing batch (%s, %s)', batch_from_dttm.isoformat(), batch_to_dttm.isoformat())

                _update_progressbar(
                    message=f'Uploading https://yt.yandex-team.ru/{yt.config["proxy"]["url"]}/navigation?path={table_name}',
                    value=round((total_seconds - (max_dttm - batch_from_dttm).total_seconds()) / total_seconds, 2),
                )

                data = await async_generator_to_list(
                    _read_solomon_data(
                        solomon_client=solomon_client,
                        project=solomon_project,
                        selectors=solomon_selectors,
                        expression=solomon_expression,
                        from_dttm=batch_from_dttm,
                        to_dttm=batch_to_dttm,
                        partition_labels=partition_labels,
                    ),
                )

                LOG.info('Starting uploading to YT')

                yt.write_table(
                    table=yt.TablePath(table_name, append=True),
                    format=yt.JsonFormat(attributes={"encode_utf8": False}),
                    input_stream=data,
                )

            if yt_table_ttl:
                LOG.debug('Setting ttl %s for table %s', yt_table_ttl, table_name)
                yt.set(f'{table_name}/@expiration_timeout', yt_table_ttl)

        from_dttm = to_dttm + ONE_SECOND
        to_dttm = YtSplitIntervals.ceil_to_interval(dttm=from_dttm, interval=yt_tables_split_interval)


async def run_operation():
    job_context = nv.context()
    mr_inputs = job_context.get_mr_inputs()
    params = job_context.get_parameters()

    yt_destination_folder = mr_inputs.get('dst').get_path()
    yt_table_ttl = params.get('yt-table-ttl', None)
    yt_tables_split_interval = params['yt-tables-split-interval'].strip()
    YtSplitIntervals.check_interval(yt_tables_split_interval)

    yt.config['token'] = params['yt-token']
    yt.config['proxy']['url'] = job_context.get_mr_cluster().get_name()
    yt.config['write_parallel']['enable'] = True
    yt.config['write_parallel']['unordered'] = True
    yt.config['write_parallel']['max_thread_count'] = 100

    solomon_endpoint = params['solomon-endpoint'].strip()
    solomon_token = params['solomon-token']
    solomon_project = params['solomon-project'].strip()
    solomon_selectors = params['solomon-selectors'].strip()
    solomon_expression = params.get('solomon-expression', '').strip() or '{selectors}'
    solomon_default_from_dttm = datetime.datetime.strptime(params['solomon-default-from-dttm'].strip(), '%Y-%m-%dT%H:%M:%S%z').astimezone(MSK_TIMEZONE_OBJ)
    solomon_page_size = params['solomon-page-size']
    solomon_partition_by_label = params.get('solomon-partition-by-labels', [])
    solomon_now_lag = params['solomon-now-lag']

    LOG.info('Starting upload selector %s to folder %s', solomon_selectors, yt_destination_folder)

    async with SolomonApiClient(endpoint=solomon_endpoint, token=solomon_token, user_agent=job_context.get_meta().get_block_url()) as solomon_client:
        await _upload(
            solomon_project=solomon_project,
            solomon_selectors=solomon_selectors,
            solomon_expression=solomon_expression,
            yt_destination_folder=yt_destination_folder,
            yt_tables_split_interval=yt_tables_split_interval,
            yt_table_ttl=yt_table_ttl,
            solomon_default_from_dttm=solomon_default_from_dttm,
            solomon_now_lag=solomon_now_lag,
            solomon_partition_by_label=solomon_partition_by_label,
            solomon_page_size=solomon_page_size,
            solomon_client=solomon_client,
        )

    LOG.info('Done!')
