# coding: utf-8
import os
import bisect
from collections import defaultdict
from datetime import datetime, timedelta

import numpy as np

import yt.wrapper as yt

from antiadblock.tasks.tools.yt_utils import ensure_yt_dynamic_table
from antiadblock.tasks.tools.solomon import get_data_from_solomon
from antiadblock.tasks.tools.misc import chuncker


SOLOMON_TIME_FORMAT = '%Y-%m-%dT%H:%M:%S.000Z'
YT_TOKEN = os.getenv('YT_TOKEN')
TABLE_PATH = '//home/antiadb/dashboard/pcode_versions'


def check_bad_share(array, mean, stdev, count, fix_treshold=0.01):
    """
    Проверяет наличие в ряде count плохих точек (+/- 3 * stdev от mean или fix_treshold)
    В отличие от разладок проверяет отдельно верхний и нижний пороги
    """

    threshold = max(3 * stdev, fix_treshold)
    lower_threshold = mean - threshold
    array.sort()
    # Все точки меньше lower_threshold
    lower_bad_count = len(array[:bisect.bisect_left(array, lower_threshold)])

    return lower_bad_count > count


def is_timeseries_not_empty(ts):
    return ts and not all(map(lambda i: i == 'NaN', ts))


def replace_nans(data):
    filtered_data = filter(lambda i: i != 'NaN', data)
    if filtered_data:
        mean = np.mean(filtered_data)
        filtered_data = [mean if i == 'NaN' else i for i in data]
    return filtered_data


def prepare_rows(services_data):
    rows = list()
    for service, data in services_data.iteritems():
        for idx, version_data in enumerate(data['top_versions']):
            version = version_data[0]
            if data.get(version) is None:
                # Так как при расчете ТОП-версий мы используем try_to_load
                # может получится, что версия есть в ТОП-5, но событий confirm_block или try_to_render для нее нет
                continue
            rows.append({
                'service_id': '{}'.format(service),
                'version': '{}'.format(version),
                'version_position': idx + 1,
                'status': data[version]['status'],
                'description': 'version: {}, action count: {}\n'
                               'previous bamboozled: {:.2%}\n'
                               'current_bamboozled: {:.2%}'.format(version, version_data[1],
                                                                   data['train_mean'], data[version]['mean'])
            })
    return rows


def calculate_train_data(solomon_data):
    """
        Считает среднее и стандартное отклонение посервисно
        :param solomon_data: raw data from Solomon
        :return: {service_id : {train_mean: mean, train_stdev" stdev}
    """
    train_data = defaultdict(dict)
    for line in solomon_data:
        if line['timeseries']['values']:
            service_id = line['timeseries']['labels']['service_id']
            # Предварительно отфильтруем значения bamboozled больше 100% и уберем все NaN
            bamboozled_data = filter(lambda value: value <= 1, replace_nans(line['timeseries']['values']))
            if bamboozled_data:
                train_data[service_id]['train_mean'] = np.mean(bamboozled_data)
                train_data[service_id]['train_stdev'] = np.std(bamboozled_data)
    return train_data


def calculate_versions_data(train_data):
    """
        Добавляет текущие данные по версиям
    :param train_data: calculate_train_data()
    :return: {service_id : {train_mean: mean, train_stdev" stdev, top_versions: [(version, action_count), ...],
    version1: mean1, ...}
    """
    for service_id, data in train_data.iteritems():
        # Находим ТОП-5 версий ПКОДа по каждому сервису за последние 12 часов
        # Считаем посервисно, так как точек очень много
        program = "{{host='cluster',service='bamboozled',cluster='cryprox-prod',sensor='bamboozled_by_pcode', " \
                  "action='try_to_load',service_id='{}',version='*'}}".format(service_id)
        downsampling = {'aggregation': 'SUM', 'gridMillis': 600 * 1000, 'fill': 'PREVIOUS'}
        start_date = (now - timedelta(minutes=12 * 60)).strftime(SOLOMON_TIME_FORMAT)
        end_date = now.strftime(SOLOMON_TIME_FORMAT)
        top_versions_data = get_data_from_solomon(program, downsampling, start_date, end_date)
        # Выкидываем пустые ряды
        top_versions_data = filter(lambda d: is_timeseries_not_empty(d['timeseries']['values']), top_versions_data)
        # Суммируем количество событий по версии. ТОП-5 по событиям добавляем к данным
        top_versions = list()
        for version in top_versions_data:
            action_count = int(sum(filter(lambda c: c != 'NaN', version['timeseries']['values'])))
            top_versions.append((version['timeseries']['labels']['version'], action_count))
        data.update({'top_versions': sorted(top_versions, key=lambda i: i[1], reverse=True)[:5]})
        # Считаем разблок по каждой из версий и добавляем среднее
        for version in map(lambda v: v[0], data['top_versions']):
            program_template = "{{host='cluster',service='bamboozled',cluster='cryprox-prod',sensor='bamboozled_by_pcode', " \
                               "action='confirm_block',service_id='{service_id}',version='{version}'}} /" \
                               "{{host='cluster',service='bamboozled',cluster='cryprox-prod',sensor='bamboozled_by_pcode', " \
                               "action='try_to_render',service_id='{service_id}',version='{version}'}}"
            bamboozled_data = get_data_from_solomon(program_template.format(service_id=service_id, version=version),
                                                    {'aggregation': 'SUM', 'gridMillis': 300 * 1000, 'fill': 'PREVIOUS'},
                                                    (now - timedelta(minutes=12 * 60)).strftime(SOLOMON_TIME_FORMAT),
                                                    now.strftime(SOLOMON_TIME_FORMAT))
            if bamboozled_data:
                bamboozled_data = replace_nans(bamboozled_data[0]['timeseries']['values'])
                bamboozled_mean = np.mean(bamboozled_data)
                bamboozled_status = 'red' if check_bad_share(bamboozled_data, data['train_mean'], data['train_stdev'], len(bamboozled_data) * 0.8) else 'green'
                data.update({version: {'mean': bamboozled_mean,
                                       'status': bamboozled_status}})
    return train_data


if __name__ == '__main__':
    now = datetime.now()
    program = "group_by_labels({host='cluster', service='bamboozled', cluster='cryprox-prod', sensor='bamboozled', " \
              "action='confirm_block', service_id='*', app='ALL', browser='ALL', device='*'}, 'service_id', v -> group_lines('sum', v))/" \
              "group_by_labels({host='cluster', service='bamboozled', cluster='cryprox-prod', sensor='bamboozled', " \
              "action='try_to_render', service_id='*', app='ALL', browser='ALL', device='*'}, 'service_id', v -> group_lines('sum', v))"
    downsampling = {'aggregation': 'SUM', 'gridMillis': 600 * 1000, 'fill': 'PREVIOUS'}
    start_date = (now - timedelta(days=7)).strftime(SOLOMON_TIME_FORMAT)
    # Последние 12 часов будем использовать для оценки разладки,
    # поэтому из этих данных их надо выкинуть
    end_date = (now - timedelta(minutes=12 * 60)).strftime(SOLOMON_TIME_FORMAT)
    train_data = get_data_from_solomon(program, downsampling, start_date, end_date)
    assert train_data is not None

    services_data = calculate_train_data(train_data)
    services_data = calculate_versions_data(services_data)

    # Вставляем данные в таблицу в YT
    yt_client = yt.YtClient(token=YT_TOKEN, proxy='hahn')
    table_schema = [
        {'name': 'service_id', 'type': 'string', 'sort_order': 'ascending'},
        {'name': 'version_position', 'type': 'uint8', 'sort_order': 'ascending'},
        {'name': 'version', 'type': 'string'},
        {'name': 'status', 'type': 'string'},
        {'name': 'description', 'type': 'string'},
    ]

    ensure_yt_dynamic_table(yt_client, TABLE_PATH, table_schema)
    rows = prepare_rows(services_data)
    for chunk in chuncker(rows, chunk_size=50):
        yt_client.insert_rows(TABLE_PATH, chunk, raw=False, format=yt.JsonFormat(attributes={"encode_utf8": False}))
