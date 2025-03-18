# -*- coding: utf8 -*-
import os
from datetime import datetime as dt
from multiprocessing import Process
import antiadblock.tasks.bypass_uids.lib.util as util
util.place_matplotlibrc()  # noqa

import click
import pandas as pd

import yt.wrapper as yt

from antiadblock.tasks.tools.logger import create_logger
import antiadblock.tasks.bypass_uids.lib.model as lib_model
import antiadblock.tasks.tools.common_configs as configs
import antiadblock.tasks.bypass_uids.lib.data as data

YT_CLUSTER = 'hahn'
logger = create_logger(__file__)


@click.group()
@click.option("--logs_range", nargs=2, metavar='xxxx-x-x', help="период, логи за который нужно использовать (Y-m-d)")
@click.option('--stream', default=False, is_flag=True, help='использовать стримовые логи для создания датасета и выборки (только для predict)')
@click.pass_context
def main(ctx, logs_range, stream):
    """
    генерирует sbr:ANTIADBLOCK_BYPASS_UIDS_DESKTOP и sbr:ANTIADBLOCK_BYPASS_UIDS_MOBILE
    с сортированными списками yandexuid-ов пользователей без блокировщиков
    наличие блокировщика предсказывается Catboost-моделью sbr:ANTIADBLOCK_BYPASS_UIDS_MODEL

    сценарии использования:

    1. обучение новой модели (обновление sbr:ANTIADBLOCK_BYPASS_UIDS_MODEL)

    ./ANTIADBLOCK_BYPASS_UIDS_TOOL --logs_range 2020-1-1 2020-1-2 dataset --train && \
    ./ANTIADBLOCK_BYPASS_UIDS_TOOL --logs_range 2020-1-1 2020-1-2 model --train --upload

    таким образом создается таблица в hahn`home/antiadb/bypas_uids_model/train_data/` с данными для обучения модели,
    модель обучется, проверяется ее Accuracy и Recall, при прохождении проверки загружается как Sandbox Resource

    2. генерация массивов yandexuid-ов (обновление sbr:ANTIADBLOCK_BYPASS_UIDS_DESKTOP и sbr:ANTIADBLOCK_BYPASS_UIDS_MOBILE)

    ./ANTIADBLOCK_BYPASS_UIDS_TOOL --logs_range 2020-1-3 2020-1-5 dataset --predict && \
    ./ANTIADBLOCK_BYPASS_UIDS_TOOL --logs_range 2020-1-3 2020-1-5 predict --upload

    таким образом создается таблица в hahn`home/antiadb/bypas_uids_model/predict_data/`
    с данными для предсказния наличия блокировщика
    последняя загруженная как sbr:ANTIADBLOCK_BYPASS_UIDS_MODEL модель используется, чтобы построить массивы yandexuid-ов
    пользователей без блокировщиков, после этого они загружаются как sbr:ANTIADBLOCK_BYPASS_UIDS_DESKTOP и
    sbr:ANTIADBLOCK_BYPASS_UIDS_MOBILE

    3. обновление таблицы с уидами из стримовых логов NGINX

    ./ANTIADBLOCK_BYPASS_UIDS_TOOL update_nginx_uids --uid_from_dt 2021-02-02T09:00:00

    таким образом обновляется таблица на hahn.`//home/antiadb/bypas_uids_model/nginx_uids/last`,
    из нее будут удаляться записи, которых _timestamp раньше чем uid_from_dt
    и будут добавлены новые записи из последнеей таблицы стримовых логов NGINX

    4. генерация массивов yandexuid-ов (обновление sbr:ANTIADBLOCK_BYPASS_UIDS_DESKTOP и sbr:ANTIADBLOCK_BYPASS_UIDS_MOBILE)

    ./ANTIADBLOCK_BYPASS_UIDS_TOOL make_bypass_uids --use_predict_uids --use_not_blocked_uids --remove_blocked_uids --upload

    В зависимости от ключей будут использованы выборка predict_uid и/или выборка not_blocked уидов из логов NGINX,
    а также исключены из выборки blocked уиды, можно указать минимальное кол-во bamboozled событий для учета уида
    и коэф-т для определения статуса not_blocked или blocked
    """
    ctx.obj = dict()
    ctx.obj['yt_token'] = os.getenv('YT_TOKEN')
    assert ctx.obj['yt_token'] is not None
    assert os.getenv('SB_TOKEN') is not None
    ctx.obj['stream'] = stream
    if logs_range:
        start, end = map(lambda t: dt.strptime(t, configs.YT_TABLES_STREAM_FMT if stream else configs.YT_TABLES_DAILY_FMT),
                         logs_range)
        assert start <= end
        ctx.obj['logs_range'] = (start, end)

    ctx.obj['yql_client'], ctx.obj['yt_client'] = util.create_clients(token=ctx.obj['yt_token'], db=YT_CLUSTER)
    data.clean_up_yt_dirs(ctx.obj['yt_client'])


@main.command('update_nginx_uids')
@click.option("--uid_from_dt", nargs=1, metavar='yyyy-mm-ddThh:mm:ss', help="Дата/время для удаления старых уидов из таблицы")
@click.pass_context
def update_nginx_uids(ctx, uid_from_dt):
    """сделать выборку уидов из стримовых логов NGINX"""
    logger.info('update uids from stream nginx log')
    resp = data.update_nginx_uids(ctx.obj['yql_client'], ctx.obj['yt_client'], uid_from_dt)
    if not resp.is_success:
        msg = '\n'.join([str(err) for err in resp.get_errors()])
        logger.error(msg=msg)
        raise Exception(msg)
    logger.info(resp)



@main.command('make_bypass_uids')
@click.option('--use_predict_uids', default=False, is_flag=True, help='использользовать подготовленную моделью выборку')
@click.option('--use_stream_predicts', default=False, is_flag=True, help='дополнительно использовать выборку, полученную из стримовых логов')
@click.option('--use_not_blocked_uids', default=False, is_flag=True, help='использользовать выборку not_blocked уидов из логово NGINX')
@click.option('--remove_blocked_uids', default=False, is_flag=True, help='удалить из подготовленной моделью выборки уиды, которые в логах NGINX помеченны с блокировщиком')
@click.option('--remove_uids_without_cycada', default=False, is_flag=True, help='удалить из подготовленной моделью выборки уиды, запросы которых без куки cycada')
@click.option('--ratio', default=0.9, help='коэффициент для определения not_blocked/blocked для уида')
@click.option('--min_cnt', default=50, help='минимальное кол-во событий для определения not_blocked/blocked для уида')
@click.option('--upload', default=False, is_flag=True, help='загрузить в Sandbox список uniqid без Адблока')
@click.pass_context
def make_bypass_uids(ctx, use_predict_uids, use_stream_predicts, use_not_blocked_uids, remove_blocked_uids, remove_uids_without_cycada, ratio, min_cnt, upload):
    """создать таблицу с уидами на основе предсказанной выборки и выборки уидов из стримовых логов NGINX"""
    if not use_predict_uids and not use_not_blocked_uids:
        raise Exception('Must use at least one of the parameters: use_predict_uids or/and use_not_blocked_uids')

    def _process(device):
        logger.info(f'make bypass uids from predict uids and preselected nginx uids for {device}')
        yql_client, yt_client = util.create_clients(token=ctx.obj['yt_token'], db=YT_CLUSTER)
        results_table_path = f'//{data.PREDICTED_UIDS_PATH}/bypass_{device}'
        resp = data.make_bypass_uids(yql_client, yt_client,
                                     results_table_path, device, ratio, min_cnt,
                                     use_predict_uids, use_stream_predicts, use_not_blocked_uids, remove_blocked_uids, remove_uids_without_cycada)

        if not resp.is_success:
            msg = '\n'.join([str(err) for err in resp.get_errors()])
            logger.error(msg=msg)
            raise Exception(msg)
        logger.info(resp)
        # upload resources
        if upload:
            util.upload_uniqids(
                yt_client,
                table_path=results_table_path,
                resource_type=data.UIDS_RESOURCE_TYPE[device],
                limit=data.UIDS_COUNT_LIMIT[device],
                ttl='3',
            )

    processes_list = []
    for device in (data.Device.desktop.name, data.Device.mobile.name):
        p = Process(target=_process, args=(device, ), name=f'{device}')
        p.start()
        processes_list.append(p)

    for process in processes_list:
        logger.info('Wait process {}...'.format(process.name))
        process.join()


@main.command()
@click.option('--train', default=False, is_flag=True, help='создать на YT датасет для обучения')
@click.option('--predict', default=False, is_flag=True, help='создать на YT датасет для предсказания uniqid без Адблока')
@click.pass_context
def dataset(ctx, train, predict):
    """создать датасет для обучения/предскзания"""
    if train:
        logger.info('preparing training dataset')
        response = data.prepare_dataset(ctx.obj['yql_client'], ctx.obj['yt_client'], date_range=ctx.obj['logs_range'], train=True)
        logger.info(response)
    if predict:
        logger.info('preparing prediction dataset')
        response = data.prepare_dataset(ctx.obj['yql_client'], ctx.obj['yt_client'], date_range=ctx.obj['logs_range'], train=False,
                                        stream=ctx.obj['stream'])
        logger.info(response)


@main.command()
@click.option('--sample_percent', default=100, help='использовать часть данных')
@click.option('--upload', default=False, is_flag=True, help='загрузить в Sandbox список uniqid без Адблока')
@click.pass_context
def predict(ctx, sample_percent, upload):
    """предскзать uniqid пользователей без Адблока"""

    def _process(device):
        yql_client, yt_client = util.create_clients(token=ctx.obj['yt_token'], db=YT_CLUSTER)
        results_table_path = os.path.join(
            data.PREDICTED_UIDS_STREAM_PATH if ctx.obj['stream'] else data.PREDICTED_UIDS_PATH,
            device,
            '_'.join([dt.strftime(d, configs.YT_TABLES_STREAM_FMT if ctx.obj['stream'] else configs.YT_TABLES_DAILY_FMT) for d in ctx.obj['logs_range']]),
        )
        logger.info('predicting bypass uids')
        response = data.predict_bypass_uids(
            yql_client,
            date_range=ctx.obj['logs_range'],
            percent=sample_percent,
            threshold=lib_model.THRESHOLD,
            device=device,
            results_path=results_table_path,
            stream=ctx.obj['stream'],
        )
        logger.info(response)
        if upload:
            util.upload_uniqids(
                yt_client,
                table_path='//' + results_table_path,
                resource_type=data.UIDS_RESOURCE_TYPE[device],
                limit=data.UIDS_COUNT_LIMIT[device],
            )

    processes_list = []
    for device in (data.Device.desktop.name, data.Device.mobile.name):
        p = Process(target=_process, args=(device,), name=f'{device}')
        p.start()
        processes_list.append(p)

    for process in processes_list:
        logger.info('Wait process {}...'.format(process.name))
        process.join()


@main.command()
@click.option('--train', default=False, is_flag=True, help='обучить catboost модель')
@click.option('--sample_percent', default=100, help='использовать часть данных')
@click.option('--resource_id', default=None, help='использовать модель с указанным Sandbox ResourceID')
@click.option('--accuracy_local', default=False, is_flag=True, help='проверить точность модели локально')
@click.option('--accuracy_yql', default=False, is_flag=True, help='проверить точность модели в YQL')
@click.option('--upload', default=False, is_flag=True, help=f'загрузить модель с Sandbox ResourceType {lib_model.RESOURCE_TYPE}')
@click.pass_context
def model(ctx, train, sample_percent, resource_id, accuracy_local, accuracy_yql, upload):
    """обучить/проверить/сохранить модель"""
    m = lib_model.ModelWrapper()
    ctx.obj['resource_id'] = resource_id

    if train or accuracy_local:
        table = yt.TablePath(
            name=os.path.join('//', data.TRAIN_DATASET_PATH, '_'.join([dt.strftime(d, configs.YT_TABLES_DAILY_FMT) for d in ctx.obj['logs_range']])),
            columns=data.CAT_FEATURES + data.FLOAT_FEATURES + data.TARGET_FEATURES,
        )
        rows = ctx.obj['yt_client'].read_table(
            table,
            format=yt.JsonFormat(attributes={'encode_utf8': False}),
            raw=False,
            table_reader=None if sample_percent == 100 else {"sampling_seed": 42, "sampling_rate": sample_percent / 100.},
        )
        train_df = pd.DataFrame(list(rows))
        logger.info(f'train_df.head(10)\n{train_df.head(10)}\ntrain_df.shape\n{train_df.shape}\ntrain_df.columns\n{train_df.columns}')
        if train:
            logger.info('training model')
            m.train(train_df)
            logger.info('saving model as Sandbox resource')
            ctx.obj['resource_id'] = m.save()
        elif ctx.obj['resource_id'] is not None:
            logger.info(f'loading model from Sandbox {ctx.obj["resource_id"]}')
            m.load(resource_id=ctx.obj['resource_id'])
            logger.info('calculating loaded model accuracy')
        else:
            raise Exception('dont have Model to test accuracy local')

        if accuracy_local:
            logger.info('calculating model accuracy locally')
            results = []
            for device in (data.Device.all.name, data.Device.desktop.name, data.Device.mobile.name):
                accuracy_df = m.evaluate_accuracy(train_df, threshold=lib_model.THRESHOLD, device=device)
                results.append(accuracy_df)
            accuracy_results = pd.concat(results)
            logger.info(f'got accuracy testing results locally:\n{accuracy_results}')

    if accuracy_yql or upload:
        if ctx.obj['resource_id'] is None:
            raise Exception('dont have Model to test accuracy in YQL')
        logger.info('calculating loaded model accuracy in YQL')
        results = []
        for device in (data.Device.all.name, data.Device.desktop.name, data.Device.mobile.name):
            accuracy_df = data.evaluate_model_accuracy(
                ctx.obj['yql_client'],
                date_range=ctx.obj['logs_range'],
                percent=sample_percent,
                resource_id=ctx.obj['resource_id'],
                threshold=lib_model.THRESHOLD,
                device=device,
            )
            accuracy_df.index = [device]
            results.append(accuracy_df)
        accuracy_results = pd.concat(results)
        logger.info(f'got accuracy testing results from YQL:\n{accuracy_results}')

        if upload:
            if accuracy_results.loc[data.Device.all.name, "accuracy"] >= lib_model.MIN_ACCURACY and accuracy_results.loc[data.Device.all.name, "recall"] >= lib_model.MIN_RECALL:
                logger.info(f'model passed validation, uploading as {lib_model.RESOURCE_TYPE}')
                if train:
                    m.save(validated=True)
                else:
                    m.load(resource_id=ctx.obj['resource_id'])
                    m.save(validated=True)
            else:
                logger.info(f'cant upload model as {lib_model.RESOURCE_TYPE}')
                logger.info(f'accuracy {accuracy_results.loc[data.Device.all.name, "accuracy"]} (min is {lib_model.MIN_ACCURACY})')
                logger.info(f'recall {accuracy_results.loc[data.Device.all.name, "recall"]} (min is {lib_model.MIN_RECALL})')
