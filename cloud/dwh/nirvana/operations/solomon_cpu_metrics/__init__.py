"""
2022-04-27

Скрипт для загрузки CPU метрик из Solomona. На момент написания - он экспортирует cpu_usage и cpu_utilization.
Он написан на основе операции solomon_to_yt, но ввиду того, что в какой-то момент ответвлений от изначальной операции
стало слишком много - было принято решение вынести код в отдельную операцию. Скрипт написан для разовой выгрузки и
выгружает информация по батчу instance_id всю историю.

Концептуально можно разделить его на следующие части -

    1. Выгрузка всех host-ов по заданным селекторам
    2. Выгрузка всех instance_id по выгруженным host-ам
    3. Выгрузка  всех уже загруженных в yt instance_id
    4. Вычитание из обзего списка instance_id тех, что уже загрузили в yt
    5. Обрезаем `n` instance_id из оставшего списка
    6. Дробим обрезанный список на батчи по `m` штук
    7. Иттерируясь по батчам, загружаем их в yt
    8. Обновляем таблицу с уже загруженными instance_id

`m` и `n` при это подбираются индивидуально каждый раз ввиду ограничения на 640 метрик из solomon и таймаута в ~ 6 часов
в nirvana.

При том для cpu_usage можно использовать довольно большой размер батчей (на текущую дату я грузил батчами
по 300 instance_id), а вот для cpu_utilization приходилось постепенно уменьшать размер батча от 50 до 1.
И даже при размере батча в 1 instance_id осталось порядка 1000 instance_id, которые выходили за лимит в 640 метрик.
Было принято решение не городить в операции еще кучу кода и загрузить их разово вручную.

Тут довольно низкое качетсво кода, но ввиду того, что загрузка разовая и на нее было потрачено и так много времени -
приняли решение оставить все как есть.
"""


from cloud.dwh.utils.log import setup_logging

import asyncio
import dataclasses
import datetime
import logging
from typing import AsyncGenerator
from typing import List
import requests

import nirvana.mr_job_context as nv
import tenacity
import yt.wrapper as yt

from cloud.dwh.clients.solomon import SolomonApiClient
from cloud.dwh.utils.coroutines import async_generator_to_list
from cloud.dwh.utils.coroutines import parallel_limiter
from cloud.dwh.utils.datetimeutils import MSK_TIMEZONE_OBJ, DAY, dttm_paginator, get_now_msk

LOG = logging.getLogger(__name__)
ONE_SECOND = datetime.timedelta(seconds=1)
LABELS_LIMIT = 100000
HEADERS = {
    'Content-Type': 'application/json',
    'User-Agent': 'YC DWH',
    'Accept': 'application/json',
    'Authorization': 'OAuth {TOKEN}'
}
LABEL_URL = 'https://solomon.yandex-team.ru/api/v2/projects/yandexcloud/sensors/labels'
SOLOMON_BATCH_SIZE = 500


@dataclasses.dataclass
class Label:
    name: str
    value: str


PartitionLabelsList = List[List[Label]]

retry = tenacity.retry(
    stop=tenacity.stop_after_attempt(5),
    wait=tenacity.wait_random(0, 3),
    reraise=True,
)


async def _read_solomon_data(solomon_client: SolomonApiClient,
                             project: str,
                             selectors: str,
                             expression: str,
                             from_dttm: datetime.datetime,
                             to_dttm: datetime.datetime,
                             partition_labels) -> AsyncGenerator[dict, None]:
    coros = []
    get_data_func = parallel_limiter(SOLOMON_BATCH_SIZE)(retry(solomon_client.data))

    params = {
        'from': int(from_dttm.timestamp() * 1000),
        'program': expression.format(selectors=selectors[:-1] + f",instance_id=~'{'|'.join(partition_labels)}'}}"),
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


async def _get_partition_labels(solomon_client: SolomonApiClient,
                                project: str,
                                label_names: List[str],
                                selectors: str = '',
                                limit: int = LABELS_LIMIT,
                                solomon_token: str = '') -> PartitionLabelsList:
    if not label_names:
        return []

    params = {'names': label_names, 'limit': limit, 'selectors': selectors}
    result = await solomon_client.get_labels(project=project, params=params)

    labels_data = {}

    for label_info in result['labels']:
        if label_info['truncated']:
            raise Exception(f'Label {label_info["name"]} is truncated for limit {limit}')

        if label_info['name'] not in label_names:
            LOG.warning('Unexpected label %s. Expected one of %s', label_info['name'], label_names)
            continue

        labels_data[label_info['name']] = [Label(name=label_info['name'], value=v) for v in label_info['values']]

    if set(label_names) - set(labels_data):
        raise Exception(f'Label(s) not found: {set(label_names) - set(labels_data)}')

    LOG.info('[CPU METRIC] getting instance ids')
    instances = []
    LOG.info('[CPU METRIC] number of hosts: %s', len(list(labels_data.values())[0]))

    hosts = [host.value for host in list(labels_data.values())[0]]
    LOG.info('[CPU METRIC] number of hosts: %s', len(set(hosts)))
    host_chunks = [hosts[x:x + 100] for x in range(0, len(hosts), 100)]

    LOG.info('[CPU METRIC] number of host_chunks: %s', len(host_chunks))
    LOG.info('SELECTORS - %s', selectors)
    for host in host_chunks:
        params_for_instances = {
            'names': ['instance_id'],
            'limit': 100000,
            'selectors': str(selectors)[1:-1] + f',host=~{"|".join(host)}'
        }
        HEADERS['Authorization'] = HEADERS['Authorization'].format(TOKEN=solomon_token)
        data = requests.get(url=LABEL_URL, headers=HEADERS, params=params_for_instances)
        try:
            LOG.info('[CPU METRIC] len - %s', len(data.json()['labels'][0]['values']))
            instances += data.json()['labels'][0]['values']
        except Exception as e:
            LOG.error(e)

    LOG.info('[CPU METRIC] Done!')
    return list(set(instances))


async def _upload(solomon_project: str,
                  solomon_selectors: str,
                  solomon_expression: str,
                  yt_destination_folder: str,
                  yt_table_ttl: int,
                  solomon_default_from_dttm: datetime.datetime,
                  solomon_now_lag: int,
                  solomon_partition_by_label: List[str],
                  solomon_client: SolomonApiClient,
                  solomon_token: str):
    partition_labels = await _get_partition_labels(
        solomon_client=solomon_client,
        project=solomon_project,
        selectors=solomon_selectors,
        label_names=solomon_partition_by_label,
        solomon_token=solomon_token
    )

    LOG.info('[CPU METRIC] number of instances: %s', len(partition_labels))
    LOG.info('[CPU METRIC] number of unique instances: %s', len(set(partition_labels)))

    yt_destination_folder = f'{yt_destination_folder}'

    if not yt.exists(yt_destination_folder):
        yt.create('map_node', yt_destination_folder, recursive=True)

    from_dttm = solomon_default_from_dttm

    max_dttm = get_now_msk() - datetime.timedelta(seconds=solomon_now_lag)

    LOG.info('Uploading solomon data from %s to %s', from_dttm.isoformat(), max_dttm.isoformat())

    transaction_attributes = {'title': f'Nirvana Operation: {nv.context().get_meta().get_block_url()}'}
    to_dttm = datetime.datetime(2022, 4, 12, tzinfo=MSK_TIMEZONE_OBJ)

    solomon_page_size = DAY*365*5

    table_name = f'''{yt_destination_folder}/{solomon_selectors.split('=')[-1].replace("'", "").replace('}', '')}'''
    already_uploaded_instances_table = f'''{yt_destination_folder}/already_uploaded_instances'''

    with yt.Transaction(attributes=transaction_attributes):
        for batch_from_dttm, batch_to_dttm in dttm_paginator(from_dttm, to_dttm, page_size=solomon_page_size):
            LOG.debug('Processing batch (%s, %s)', batch_from_dttm.isoformat(), batch_to_dttm.isoformat())

            LOG.info('[CPU METRIC] Get already uploaded instances')
            already_uploaded_instances = set()
            for row in yt.read_table(already_uploaded_instances_table, format=yt.JsonFormat()):
                already_uploaded_instances.add(row['instance_id'])
            LOG.info('[CPU METRIC] Number of already uploaded instances = %s', len(already_uploaded_instances))

            partition_labels = list(set(partition_labels) - already_uploaded_instances)
            LOG.info('[CPU METRIC] Number of do not uploaded instances - %s', len(partition_labels))
            partition_labels = partition_labels[:50000]

            instance_chunks = [partition_labels[x:x + 1] for x in range(0, len(partition_labels), 1)]
            LOG.info('[CPU METRIC] Number of chunks - %s', len(instance_chunks))
            i = 0
            err = []
            for chunk in instance_chunks:
                LOG.info('[CPU METRIC] Downloading chunk %s/%s', i, len(instance_chunks))
                try:
                    data = await async_generator_to_list(
                        _read_solomon_data(
                            solomon_client=solomon_client,
                            project=solomon_project,
                            selectors=solomon_selectors,
                            expression=solomon_expression,
                            from_dttm=batch_from_dttm,
                            to_dttm=batch_to_dttm,
                            partition_labels=chunk,
                        ),
                    )
                except Exception as e:
                    LOG.error(e)
                    data = None
                    err += chunk
                if data:
                    LOG.info('[CPU METRIC] Uploading chunk %s/%s', i, len(instance_chunks))
                    yt.write_table(
                        table=yt.TablePath(table_name, append=True),
                        format=yt.JsonFormat(attributes={"encode_utf8": False}),
                        input_stream=data,
                    )
                else:
                    LOG.info('[CPU METRIC] Empty data, continue')
                i += 1
            LOG.info('[CPU METRIC] Err - %s', err)
            partition_labels = list(set(partition_labels) - set(err))
            instances = [{'instance_id': x} for x in partition_labels]
            yt.write_table(
                yt.TablePath(already_uploaded_instances_table, append=True),
                instances,
                format="json"
            )

        if yt_table_ttl:
            LOG.debug('Setting ttl %s for table %s', yt_table_ttl, table_name)
            yt.set(f'{table_name}/@expiration_timeout', yt_table_ttl)


async def run_operation():
    job_context = nv.context()
    mr_inputs = job_context.get_mr_inputs()
    params = job_context.get_parameters()

    yt_destination_folder = mr_inputs.get('dst').get_path()
    yt_table_ttl = params.get('yt-table-ttl', None)

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
    solomon_default_from_dttm = datetime.datetime.strptime(params['solomon-default-from-dttm'].strip(),
                                                           '%Y-%m-%dT%H:%M:%S%z').astimezone(MSK_TIMEZONE_OBJ)
    solomon_partition_by_label = params.get('solomon-partition-by-labels', [])
    solomon_now_lag = params['solomon-now-lag']

    LOG.info('Starting upload selector %s to folder %s', solomon_selectors, yt_destination_folder)

    async with SolomonApiClient(endpoint=solomon_endpoint, token=solomon_token,
                                user_agent=job_context.get_meta().get_block_url()) as solomon_client:
        await _upload(
            solomon_project=solomon_project,
            solomon_selectors=solomon_selectors,
            solomon_expression=solomon_expression,
            yt_destination_folder=yt_destination_folder,
            yt_table_ttl=yt_table_ttl,
            solomon_default_from_dttm=solomon_default_from_dttm,
            solomon_now_lag=solomon_now_lag,
            solomon_partition_by_label=solomon_partition_by_label,
            solomon_client=solomon_client,
            solomon_token=solomon_token
        )

    LOG.info('Done!')


def main():
    setup_logging(debug=nv.context().get_parameters().get('debug-logging', False))
    asyncio.run(run_operation())


if __name__ == '__main__':
    main()
