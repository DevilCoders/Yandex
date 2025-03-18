# -*- coding: utf8 -*-
import os
import enum
from operator import attrgetter

from jinja2 import DictLoader
from jinja2.nativetypes import NativeEnvironment

from library.python import resource

import antiadblock.tasks.tools.common_configs as configs
import antiadblock.tasks.tools.yt_utils as yt_utils


class Columns(enum.Enum):
    uniqid = enum.auto()
    # target
    adblock = enum.auto()

    # categorical features
    # bs-chevent-log:
    device = enum.auto()  # desktop/mobile
    browsername = enum.auto()  # Браузер, который определился yabs-uatraits
    detaileddevicetype = enum.auto()  # Операционная система устройства
    # visit-v2-log:
    region_id = enum.auto()
    sex = enum.auto()
    visit_adblock = enum.auto()
    age = enum.auto()
    user_agent = enum.auto()  # Идентификатор браузера
    os_familiy = enum.auto()  # семейство операционных систем (Windows, Linux, MacOS, Android, iOS и т.п.)

    # float features
    # bs-chevent-log:
    shows = enum.auto()  # cуммарное количество показов
    clicks = enum.auto()  # суммарное количество кликов
    aab_shows = enum.auto()  # суммарное количество показов, размеченных как антиадблочные
    aab_clicks = enum.auto()  # суммарное количество кликов, размеченных как антиадблочные
    noaab_domains_clicks = enum.auto() # суммарное количество кликов на площадках, не использующих антиадблочные решения
    noaab_domains_shows = enum.auto()  # суммарное количество показов на площадках, не использующих антиадблочные решения
    noaab_domains_unique = enum.auto()  # количество посещенных пользователем площадок, не использующих антиадблочные решения
    # visit-v2-log:
    visits_cnt = enum.auto()  # суммарное количество визитов

    shows_ratio = enum.auto()  # отношение количества показов `shows` к количесву визитов `visits_cnt`

    def __str__(self):
        return str(self.name)

    def __repr__(self):
        return str(self.name)


class Device(enum.Enum):
    all = enum.auto()
    desktop = enum.auto()
    mobile = enum.auto()

    def __str__(self):
        return str(self.name)

    def __repr__(self):
        return str(self.name)


UIDS_RESOURCE_TYPE = {
    Device.desktop.name: 'ANTIADBLOCK_BYPASS_UIDS_' + Device.desktop.name.upper(),
    Device.mobile.name: 'ANTIADBLOCK_BYPASS_UIDS_' + Device.mobile.name.upper(),
}

UIDS_COUNT_LIMIT = {
    Device.desktop.name: 100 * 10 ** 6,
    Device.mobile.name: 200 * 10 ** 6,
}

float_features = [
    Columns.shows, Columns.clicks,
    # Columns.aab_shows, Columns.aab_clicks,  (feature_importance aab_shows довольно высок, убираем из-за того, что не все площадки подключены к Антиадблоку - не у всех uniqid будут такие данные)
    Columns.noaab_domains_shows, Columns.noaab_domains_unique,
    # Columns.noaab_domains_clicks,  (feature_importance ~ 0)
    Columns.visits_cnt, Columns.shows_ratio
]
target_features = [Columns.adblock]
cat_features = [
    Columns.device, Columns.browsername, Columns.detaileddevicetype,
    Columns.region_id, Columns.sex, Columns.visit_adblock, Columns.age, Columns.user_agent, Columns.os_familiy,
]
pass_through = [Columns.uniqid]

name = attrgetter('name')
FLOAT_FEATURES = list(map(name, float_features))
TARGET_FEATURES = list(map(name, target_features))
CAT_FEATURES = list(map(name, cat_features))
PASS_THROUGH = list(map(name, pass_through))

templates = NativeEnvironment(loader=DictLoader({
    'prepare': bytes.decode(resource.find('prepare_dataset')),
    'sample': bytes.decode(resource.find('sample_dataset')),
    'evaluate_accuracy': bytes.decode(resource.find('evaluate_accuracy')),
    'model_predict': bytes.decode(resource.find('model_predict')),
    'update_nginx_uids': bytes.decode(resource.find('update_nginx_uids')),
    'make_bypass_uids': bytes.decode(resource.find('make_bypass_uids')),
}))

TRAIN_DATASET_PATH = 'home/antiadb/bypas_uids_model/train_data'
PREDICT_DATASET_PATH = 'home/antiadb/bypas_uids_model/predict_data'
PREDICT_DATASET_STREAM_PATH = 'home/antiadb/bypas_uids_model/stream/predict_data'
PREDICTED_UIDS_PATH = 'home/antiadb/bypas_uids_model/bypass_uids'
PREDICTED_UIDS_STREAM_PATH = 'home/antiadb/bypas_uids_model/stream/bypass_uids'
NGINX_UIDS_PATH = '//home/antiadb/bypas_uids_model/nginx_uids/data'
CYCADA_STAT_PATH = '//home/antiadb/bypas_uids_model/nginx_uids/cycada_stat'
NGINX_LOG_PATH = '//logs/antiadb-nginx-log/stream/5min'


def get_last_table(yt, path):
    return os.path.join(path, sorted(yt.list(path))[-1])


def clean_up_yt_dirs(yt):
    yt_utils.remove_old_tables(yt, '//' + TRAIN_DATASET_PATH, 14)
    yt_utils.remove_old_tables(yt, '//' + PREDICT_DATASET_PATH, 14)
    yt_utils.remove_old_tables(yt, '//' + PREDICT_DATASET_STREAM_PATH, 2)
    yt_utils.remove_old_tables(yt, NGINX_UIDS_PATH, 0.25)  # 0.25 day = 6 hours
    yt_utils.remove_old_tables(yt, CYCADA_STAT_PATH, 0.25)
    for device in (Device.mobile.name, Device.desktop.name):
        yt_utils.remove_old_tables(yt, os.path.join('//' + PREDICTED_UIDS_PATH, device))
        yt_utils.remove_old_tables(yt, os.path.join('//' + PREDICTED_UIDS_STREAM_PATH, device), 2)


def update_nginx_uids(yql_client, yt_client, datetime_min):
    last_nginx_log_table = get_last_table(yt_client, NGINX_LOG_PATH).split('/')[-1]
    old_uids_table_path = get_last_table(yt_client, NGINX_UIDS_PATH)
    old_cycada_stat_table_path = get_last_table(yt_client, CYCADA_STAT_PATH)
    query = templates.get_template('update_nginx_uids').render(
        file=__file__,
        datetime_min=datetime_min,
        old_uids_table=old_uids_table_path,
        nginx_uids_path=NGINX_UIDS_PATH,
        old_cycada_stat_table=old_cycada_stat_table_path,
        cycada_stat_path=CYCADA_STAT_PATH,
        last_nginx_log=last_nginx_log_table,
    )
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    resp = yql_client.query(query, syntax_version=1, title=f'UPDATE NGINX UIDS {__file__} YQL').run().get_results()
    return resp


def make_bypass_uids(yql_client, yt_client,
                     result_table, device, ratio, min_cnt,
                     use_predict_uids, use_stream_predicts, use_not_blocked_uids, remove_blocked_uids, remove_uids_without_cycada):
    predict_uid_table_path = get_last_table(yt_client, f'//{PREDICTED_UIDS_PATH}/{device}')
    predict_uid_stream_table_path = get_last_table(yt_client, f'//{PREDICTED_UIDS_STREAM_PATH}/{device}')
    nginx_uid_table_path = get_last_table(yt_client, NGINX_UIDS_PATH)
    cycada_stat_table_path = get_last_table(yt_client, CYCADA_STAT_PATH)
    query = templates.get_template('make_bypass_uids').render(
        file=__file__,
        nginx_uid_table_path=nginx_uid_table_path,
        cycada_stat_table_path=cycada_stat_table_path,
        predict_uid_table_path=predict_uid_table_path,
        predict_uid_stream_table_path=predict_uid_stream_table_path,
        result_table=result_table,
        device=device,
        ratio=ratio,
        min_cnt=min_cnt,
        use_predict_uids=use_predict_uids,
        use_not_blocked_uids=use_not_blocked_uids,
        remove_blocked_uids=remove_blocked_uids,
        remove_uids_without_cycada=remove_uids_without_cycada,
        use_stream_predicts=use_stream_predicts,
    )
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    resp = yql_client.query(query, syntax_version=1, title=f'MAKE BYPASS UIDS {__file__} YQL').run().get_results()
    return resp


def get_dataset_sample(yql_client, date_range, percent):
    query = templates.get_template('sample').render(
        file=__file__,
        start=date_range[0], end=date_range[1],
        table_name_fmt=configs.YT_TABLES_DAILY_FMT,
        train_dataset_path=TRAIN_DATASET_PATH,
        percent=percent,
        cat_features=CAT_FEATURES,
        float_features=FLOAT_FEATURES,
        target_features=TARGET_FEATURES,
        pass_through=PASS_THROUGH,
    )
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    df = yql_client.query(query, syntax_version=1, title=f'SAMPLE DATASET {__file__} YQL').run().full_dataframe
    return df


def prepare_dataset(yql_client, yt_client, date_range, train, stream=False):
    if train:
        assert not stream
    partners_pageids_table = get_last_table(yt_client, configs.YT_ANTIADB_PARTNERS_PAGEIDS_PATH)
    no_adblock_domains_table = get_last_table(yt_client, configs.NO_ADBLOCK_DOMAINS_PATH)
    predict_path = PREDICT_DATASET_STREAM_PATH if stream else PREDICT_DATASET_PATH

    query = templates.get_template('prepare').render(
        file=__file__,
        columns=Columns, devices=Device,
        start=date_range[0], end=date_range[1],
        table_name_fmt=configs.YT_TABLES_STREAM_FMT if stream else configs.YT_TABLES_DAILY_FMT,
        partners_pageids_table=partners_pageids_table,
        results_path=TRAIN_DATASET_PATH if train else predict_path,
        no_adblock_domains_table=no_adblock_domains_table,
        train=train,
        stream=stream,
    )
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    resp = yql_client.query(query, syntax_version=1, title=f'{"TRAIN" if train else "PREDICT"} DATASET {__file__.split("/", 1)[1]} YQL').run().get_results()
    return resp


def evaluate_model_accuracy(yql_client, date_range, percent, resource_id, threshold, device=Device.all.name):
    query = templates.get_template('evaluate_accuracy').render(
        file=__file__,
        columns=Columns, devices=Device,
        start=date_range[0], end=date_range[1],
        table_name_fmt=configs.YT_TABLES_DAILY_FMT,
        dataset_path=TRAIN_DATASET_PATH,
        percent=percent,
        cat_features=CAT_FEATURES,
        float_features=FLOAT_FEATURES,
        target_features=TARGET_FEATURES,
        pass_through=PASS_THROUGH,
        resource_id=resource_id,
        threshold=threshold,
        device=device,
    )
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    df = yql_client.query(query, syntax_version=1, title=f'MODEL ACCURACY {device.upper()} {__file__.split("/", 1)[1]} YQL').run().full_dataframe
    return df


def predict_bypass_uids(yql_client, date_range, percent, threshold, results_path, device=Device.desktop.name, stream=False):
    query = templates.get_template('model_predict').render(
        file=__file__,
        columns=Columns, devices=Device,
        start=date_range[0], end=date_range[1],
        table_name_fmt=configs.YT_TABLES_STREAM_FMT if stream else configs.YT_TABLES_DAILY_FMT,
        dataset_path=PREDICT_DATASET_STREAM_PATH if stream else PREDICT_DATASET_PATH,
        percent=percent,
        cat_features=CAT_FEATURES,
        float_features=FLOAT_FEATURES,
        target_features=TARGET_FEATURES,
        pass_through=PASS_THROUGH,
        threshold=threshold,
        device=device,
        results_path=results_path,
    )
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    resp = yql_client.query(query, syntax_version=1, title=f'PREDICT {device.upper()} {__file__.split("/", 1)[1]} YQL').run().get_results()
    return resp
