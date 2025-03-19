import json
import click
import logging.config
from textwrap import dedent
from datetime import datetime, timedelta
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.data_adapters.YTAdapter import YTAdapter
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing
from ml_model_features.utils.utils import get_last_day_of_month

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
@click.command()
@click.option('--is_prod', is_flag=True, default=False)
@click.option('--rebuild', is_flag=True, default=False)
def copy_metrika_visits_raw(is_prod: bool = False, rebuild: bool = False) -> None:
    logger.info('Starting `copy_metrika_visits_raw` process...')
    yt_adapter = YTAdapter()
    yql_adapter = YQLAdapter()

    source_folder_path = '//statbox/cooked_logs/visit-cooked-log/v1/1d'
    result_folder_path = '//home/cloud_analytics/ml/ml_model_features/raw/metrika/counter_51465824/visits'
    test_table = '//home/cloud_analytics/ml/ml_model_features/raw/metrika/counter_51465824/visits_test'

    # dates to start and end
    result_table_names = yt_adapter.yt.list(result_folder_path)
    if len(result_table_names) > 0 and not rebuild:
        date_from = (datetime.today() - timedelta(days=45)).replace(day=1).strftime('%Y-%m-%d')
    else:
        date_from = '2021-01-01'
    date_end = max(yt_adapter.yt.list(source_folder_path))

    res_log = {'updated tables': []}
    while date_from <= date_end:
        date_from_dt = datetime.strptime(date_from, '%Y-%m-%d')
        month_end_str = get_last_day_of_month(date_from)
        part_date_end = min(month_end_str, date_end)

        logger.info(f'Loading part {date_from} to {part_date_end} ...')
        insert_table_name = f"{result_folder_path}/{date_from_dt.strftime('%Y-%m')}" if is_prod else test_table
        query = yql_adapter.execute_query(dedent(f'''
        PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
        PRAGMA yt.QueryCacheTtl = '1h';

        INSERT INTO `{insert_table_name}` WITH TRUNCATE
            SELECT
                DateTime::Format('%Y-%m-%d')(Cast(`UTCStartTime` AS Datetime)) AS `billing_record_msk_date`, --Дата визита
                `PassportUserID` AS `puid`, --Passport UID из куки yandex.ru из первого просмотра
                Cast(`UTCStartTime` AS Datetime) AS `session_start_time`, --Время визита
                `VisitID` AS `visit_id`, --Идентификатор визита
                `Hits` AS `hits_num`, --Число просмотров в визите
                Cast(Coalesce(`IsBounce`, false) AS UInt8) AS `is_bounce`, --Является ли визит отказом
                Coalesce(`Duration`, 0) AS `duration`, --Длительность визита в секундах
                Coalesce(`GoalReachesAny`, 0) AS `any_goal_reaches_num`, --Количество достижений целей
                Cast((Coalesce(`TraficSourceID`, -1) = -1) AS UInt8) AS `visits_iternal_links`, --Внутренние переходы
                Cast((Coalesce(`TraficSourceID`, -1) = 0) AS UInt8) AS `visits_direct`, --Прямые заходы
                Cast((Coalesce(`TraficSourceID`, -1) = 1) AS UInt8) AS `visits_from_site_links`, --Переходы по ссылкам на сайтах
                Cast((Coalesce(`TraficSourceID`, -1) = 2) AS UInt8) AS `visits_from_search`, --Переходы из поисковых систем
                Cast((Coalesce(`TraficSourceID`, -1) = 3) AS UInt8) AS `visits_from_ads`, --Переходы по рекламе
                Cast((Coalesce(`TraficSourceID`, -1) = 4) AS UInt8) AS `visits_from_saved`, --Переходы с сохранённых страниц
                Cast((Coalesce(`TraficSourceID`, -1) = 5) AS UInt8) AS `visits_unknown`, --Не определён
                Cast((Coalesce(`TraficSourceID`, -1) = 6) AS UInt8) AS `visits_external_links`, --Переходы по внешним ссылкам
                Cast((Coalesce(`TraficSourceID`, -1) = 7) AS UInt8) AS `visits_from_letters`, --Переходы с почтовых рассылок
                Cast((Coalesce(`TraficSourceID`, -1) = 8) AS UInt8) AS `visits_from_social_media`, --Переходы из социальных сетей
                Cast((Coalesce(`TraficSourceID`, -1) = 9) AS UInt8) AS `visits_from_recommenders`, --Переходы из рекомендательных систем
                Cast((Coalesce(`TraficSourceID`, -1) = 10) AS UInt8) AS `visits_from_messangers`, --Переходы из мессенджеров
                Yson::ConvertToUint64List(`Goals_ID`) AS `goals_id_list`, --Массив ид целей, достигнутых за этот визит (размера any_goal_reaches_num)
                Cast(ListHas(Yson::ConvertToUint64List(`Goals_ID`), 174723400u) AS UInt8) AS `submit_forms`, --Автоцель: отправка формы (любой)
                Cast(ListHas(Yson::ConvertToUint64List(`Goals_ID`), 174723388u) AS UInt8) AS `loaded_docs`, --Автоцель: скачивание файла
                Cast(ListHas(Yson::ConvertToUint64List(`Goals_ID`), 40789009u) AS UInt8) AS `audience_docs`, --Посещение страниц, содержащих /docs
                Cast(ListHas(Yson::ConvertToUint64List(`Goals_ID`), 40788193u) AS UInt8) AS `audience_prices`, --Посещение страниц, содержащих /prices
                Cast(ListHas(Yson::ConvertToUint64List(`Goals_ID`), 40804669u) AS UInt8) AS `audience_support`, --Посещение страниц, содержащих /support
                Coalesce(`GoalReachesDepth`, 0) AS `goal_reaches_depth`, --Количество достижений цели на глубину
                Coalesce(`GoalReachesURL`, 0) AS `goal_reaches_url`, --Количество достижений урловых целей
                Cast(Coalesce(`IsDownload`, false) AS UInt8) AS `is_download`, --Была ли загрузка в этом визите
                Cast(Coalesce(`IsLoggedIn`, false) AS UInt8) AS `is_logged_in`, --Посетитель залогирован на Яндексе
                Cast(Coalesce(`IsRobotInternal`, false) AS UInt8) AS `is_robot_internal`, --Флаг роботности от антифрода
                Cast(Coalesce(`IsTurboPage`, false) AS UInt8) AS `is_turbo_page`, --Был ли хит из Турбо страницы
                Cast(Coalesce(`IsWebView`, false) AS UInt8) AS `is_web_view`, --Был ли хит из WebView
                Cast(Coalesce(`JavaEnable`, false) AS UInt8) AS `java_enable`, --Java
                Cast(Coalesce(`JavascriptEnable`, false) AS UInt8) AS `javascript_enable`, --JavaScript
                Ip::ToString(`ClientIP6`) AS `visit_from_ip` --IPv6 (or embedded-IPv6 for IPv4 client address) which used for visit
            FROM Range(`{source_folder_path}`, `{date_from}`, `{part_date_end}`)
            WHERE `Sign` = 1 AND `CounterID` = 50027884 AND `PassportUserID` IS NOT NULL
            ORDER BY `billing_record_msk_date`, `puid`, `session_start_time`
            {'' if is_prod else 'LIMIT 100'}
        ;'''))
        query.run()
        query.get_results()
        is_yql_query_success = YQLAdapter.is_success(query)
        assert is_yql_query_success, 'Failed'
        res_log['updated tables'].append(insert_table_name)
        date_from = (datetime.strptime(part_date_end, '%Y-%m-%d') + timedelta(days=1)).strftime('%Y-%m-%d')
        if not is_prod:
            break

    with open('output.json', 'w') as fout:
        json.dump(res_log, fout)


if __name__ == "__main__":
    copy_metrika_visits_raw()
