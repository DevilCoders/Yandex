import json
import click
import logging.config
from textwrap import dedent
from clan_tools import utils
from datetime import datetime, timedelta
from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.timing import timing

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@timing
def update_logs_in_period(dt_from: str, dt_till: str):
    yql_adapter = YQLAdapter()

    query = dedent(f'''
    PRAGMA yt.Pool = 'cloud_analytics_pool';
    PRAGMA AnsiInForEmptyOrNullableItemsCollections;

    $nvl = ($x, $val) -> {{RETURN If($x is Null, $val, $x)}};

    --sql
    $COUNTER_SOURCE_SITE = (
        SELECT
            `CounterID`, --Идентификатор счетчика
            `UserID`, --Идентификатор юзера
            `PassportUserID`, --Passport UID из куки yandex.ru из первого просмотра
            `CryptaID`, --CryptaID v2
            `VisitID`, --Идентификатор визита
            `VisitVersion`, --Идентификатор чанка визита
            `Hits`, --Число просмотров в визите
            Cast($nvl(`IsBounce`, false) AS UInt8) AS `IsBounce`, --Является ли визит отказом
            $nvl(`Duration`, 0) AS `Duration`, --Длительность визита в секундах
            `UTCStartTime`, --Дата и время начала визита в UTC (дата и время первого хита в визите)
            Cast(Cast(`UTCStartTime` AS Datetime) AS Date) AS `StartDate`, --Дата визита
            `GoalReachesAny`, --Количество достижений целей
            Cast(($nvl(`TraficSourceID`, -1) = -1) AS UInt8) AS `VisitsIternalLinks`, --Внутренние переходы
            Cast(($nvl(`TraficSourceID`, -1) = 0) AS UInt8) AS `VisitsDirect`, --Прямые заходы
            Cast(($nvl(`TraficSourceID`, -1) = 1) AS UInt8) AS `VisitsFromSiteLinks`, --Переходы по ссылкам на сайтах
            Cast(($nvl(`TraficSourceID`, -1) = 2) AS UInt8) AS `VisitsFromSearch`, --Переходы из поисковых систем
            Cast(($nvl(`TraficSourceID`, -1) = 3) AS UInt8) AS `VisitsFromAds`, --Переходы по рекламе
            Cast(($nvl(`TraficSourceID`, -1) = 4) AS UInt8) AS `VisitsFromSaved`, --Переходы с сохранённых страниц
            Cast(($nvl(`TraficSourceID`, -1) = 5) AS UInt8) AS `VisitsUnknown`, --Не определён
            Cast(($nvl(`TraficSourceID`, -1) = 6) AS UInt8) AS `VisitsExternalLinks`, --Переходы по внешним ссылкам
            Cast(($nvl(`TraficSourceID`, -1) = 7) AS UInt8) AS `VisitsFromLetters`, --Переходы с почтовых рассылок
            Cast(($nvl(`TraficSourceID`, -1) = 8) AS UInt8) AS `VisitsFromSocialMedia`, --Переходы из социальных сетей
            Cast(($nvl(`TraficSourceID`, -1) = 9) AS UInt8) AS `VisitsFromRecommenders`, --Переходы из рекомендательных систем
            Cast(($nvl(`TraficSourceID`, -1) = 10) AS UInt8) AS `VisitsFromMessagers`, --Переходы из мессенджеров
            Yson::ConvertToUint64List(`Goals_ID`) AS `Goals_ID`, --Массив идентификаторов целей, достигнутых за этот визит
            Cast(ListHas(Yson::ConvertToUint64List(`Goals_ID`), 174723400u) AS UInt8) AS `SubmitAnyFormOnSiteAndPromos`, --Автоцель: отправка формы (любой)
            Cast(ListHas(Yson::ConvertToUint64List(`Goals_ID`), 174723388u) AS UInt8) AS `DownloadAnyDocOnSiteAndPromos`, --Автоцель: скачивание файла

            --Клик по кнопке "Подписаться на новости" для незарегистрированных
            Cast(ListHas(Yson::ConvertToUint64List(`Goals_ID`), 104725810u) AS UInt8) AS `SubscribeForNotMembers`,

            --Клик по кнопке "Подписаться на новости" для зарегистрированных -> переход в консоль
            Cast(ListHas(Yson::ConvertToUint64List(`Goals_ID`), 104726074u) AS UInt8) AS `EditSubscriptionsForMembers`,
            Cast(ListHas(Yson::ConvertToUint64List(`Goals_ID`), 40789009u) AS UInt8) AS `AudienceDocs`, --Посещение страниц, содержащих /docs
            Cast(ListHas(Yson::ConvertToUint64List(`Goals_ID`), 40788193u) AS UInt8) AS `AudiencePrices`, --Посещение страниц, содержащих /prices
            Cast(ListHas(Yson::ConvertToUint64List(`Goals_ID`), 40804669u) AS UInt8) AS `AudienceSupport`, --Посещение страниц, содержащих /support
            $nvl(`GoalReachesDepth`, 0) AS `GoalReachesDepth`, --Количество достижений цели на глубину
            $nvl(`GoalReachesURL`, 0) AS `GoalReachesURL`, --Количество достижений урловых целей
            Cast($nvl(`IsDownload`, false) AS UInt8) AS `IsDownload`, --Была ли загрузка в этом визите
            Cast($nvl(`IsLoggedIn`, false) AS UInt8) AS `IsLoggedIn`, --Посетитель залогирован на Яндексе
            Cast($nvl(`IsRobotInternal`, false) AS UInt8) AS `IsRobotInternal`, --Флаг роботности от антифрода
            Cast($nvl(`IsTurboPage`, false) AS UInt8) AS `IsTurboPage`, --Был ли хит из Турбо страницы
            Cast($nvl(`IsWebView`, false) AS UInt8) AS `IsWebView`, --Был ли хит из WebView
            Cast($nvl(`JavaEnable`, false) AS UInt8) AS `JavaEnable`, --Есть ли Java у посетителя
            Cast($nvl(`JavascriptEnable`, false) AS UInt8) AS `JavascriptEnable` --Включен ли яваскрипт
        FROM Range(`statbox/cooked_logs/visit-cooked-log/v1/1d`, `{dt_from}`, `{dt_till}`)
        WHERE `Sign` = 1 AND `CounterID` = 50027884
    );

    --sql
    INSERT INTO `home/cloud_analytics/ml/scoring/consumption_predictor_v2/data/marketing_daily/metrika_visits/{dt_from[:-3]}` WITH TRUNCATE
    SELECT * FROM $COUNTER_SOURCE_SITE;
    ''')

    yql_adapter.run_query(query, utils.__file__, 'yql')


@click.command()
@click.option('-n', '--num-months', default=2, type=int)
@timing
def update_last_logs(num_months: int):
    def ISO_format(dt):
        return dt.strftime('%Y-%m-%d')

    # generate monthly chunks to recalculate
    dt_beg = datetime.now().date()
    for i in range(num_months):
        dt_end = dt_beg + timedelta(days=-1)
        dt_beg = dt_end.replace(day=1)
        logger.info(f"{i+1}/{num_months}: {ISO_format(dt_beg)} to {ISO_format(dt_end)}")
        update_logs_in_period(ISO_format(dt_beg), ISO_format(dt_end))

    with open('output.json', 'w') as fp:
        json.dump({"Number of updated tables": num_months}, fp)

if __name__ == "__main__":
    update_last_logs()
