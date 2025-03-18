# Тулза Антиадблока для подсчета денег по service_id в разных срезах (только Директ, без блоков Турбо)
## Сборка и запуск
```bash
ya make --checkout

./money_by_service_id -h
usage: money_by_service_id [-h] [--yt_token TOKEN] [--stat_token TOKEN]
                           [--test] [--daily_logs] [--update_pageids]
                           [--range xxxx-xx-xx* xxxx-xx-xx*]

Tool for calculating antiadblock money and uploading results to Stat & Solomon

optional arguments:
  -h, --help            show this help message and exit
  --yt_token TOKEN      Expects oAuth token for YT as value
  --stat_token TOKEN    Expects oAuth token for Stat as value
  --test                Will send results to Stat & Solomon testing if enabled
  --daily_logs          If endbled yql request will use logs/bs-chevent-log/1d
                        istead of logs/bs-chevent-log/stream/5min
  --update_pageids      If enabled wont use cache for service_id to pageids
                        map
  --range xxxx-xx-xx* xxxx-xx-xx*
                        Range of log files in yql request. Y-m-d format if
                        --daily_logs else Y-m-dTH:M:S
```

## YQL-запрос для сбора списка имен, к которым привязаны pageid (на основе проксируемых запросов)
В `bs-chevent-logs`, которые используются данной таской для подсчета статистики, события клика/показа всегда привязаны к определенному `pageid`, список уникальных `pageid` каждого партнера находится в [тут](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/tasks/tools/common_configs.py) в переменных `SERVICE_ID_TO_PAGEID_NAMES` и `SERVICE_ID_TO_CUSTOM_PAGEID_NAMES`.
Эти переменные сформированы на основе логов рекламных запросов проходящих через прокси.
Для НЕ ADFOX запросов все довольно просто:
```sqlite
$page_bk = Pire::Capture("https?://\\w+\\.yandex\\.ru/\\w{4}/(\\d+)\\?.*");
$service_to_page = (
    SELECT
        service_id, pageid, count(*) as cnt
    FROM hahn.`home/logfeller/logs/antiadb-cryprox-log/1d/2019-03-26`
    where 
        action = 'fetch_content' and
        http_host in ('an.yandex.ru', 'yabs.yandex.ru') and
        (url like '%an.yandex.ru/meta%' or url like '%an.yandex.ru/page%' or url like '%yabs.yandex.ru/meta%' or url like '%yabs.yandex.ru/page%')
    group by service_id, cast($page_bk(url) as uint64) as pageid
);
$service_id_requests = (SELECT service_id, sum(cnt) as total from $service_to_page group by service_id);

SELECT 
    stp.service_id as service_id,
    AGGREGATE_LIST_DISTINCT(m.Name) as all_domains,
    AGGREGATE_LIST_DISTINCT(stp.pageid) as all_pageids
from 
    $service_to_page as stp left join 
    (select PageID, Name from hahn.`home/yabs/dict/Page`) as m on
    stp.pageid = cast(m.PageID as uint64)
GROUP BY stp.service_id as service_id
```
[Пример](https://yql.yandex-team.ru/Operations/XJvL0wlSshlf9dMp6w1cCrvrTXBM130P8uDps0yYhiI=)

Для ADFOX запросов, сложнее, потому что в `https://ads.adfox.ru/59610/getBulk...` число 59610 - это `owner_id`, который не имеет однозначной привязки к конкретному `pageid`.
YQL, который для каждого `service_id` выводит проксируемые `owner_id`, список привязаных к `owner_id` `pageid` и список pageid names для `pageid`:
```sqlite
$capture_owner = Pire::Capture("https?://ads\.adfox\.ru/([^/?]+)/getBulk.*");
$capture_page = Pire::Capture("\\w-\\w-(\\d+)-\\d");
$owner_to_names = (
SELECT owner_id, AGGREGATE_LIST_DISTINCT(p.Name) as names, AGGREGATE_LIST_DISTINCT(p.PageID) as pageids
from 
    (SELECT owner_id, pageid from hahn.`home/adfox/dictionaries/rsya_block` group by owner_id, $capture_page(block_id) as pageid) as owner join
    hahn.`home/yabs/dict/Page` as p on 
    p.PageID = cast(owner.pageid as uint64)
GROUP BY owner.owner_id as owner_id
);
$service_id_to_owner = (
SELECT service_id, owner_id, count(*) as cnt
FROM hahn.`home/logfeller/logs/antiadb-cryprox-log/1d/2019-03-26`
where 
    action = 'fetch_content' and
    http_host='ads.adfox.ru' and
    url like '%ads.adfox.ru/%/getBulk%'
group by 
    service_id,
    cast($capture_owner(url) as uint64) as owner_id
);

SELECT service_id, s.owner_id as owner_id, cnt, pageids, names
from $owner_to_names as n join $service_id_to_owner as s on s.owner_id = n.owner_id
order by service_id
```
[Пример](https://yql.yandex-team.ru/Operations/XJvUuDa9vAZt5MNMPDiuKwplFtszyudHmldNAKAoQk0=)

## Конфиг отчета на Стате с результатами выполнения запросов
Сейчас данные отправляются в https://stat.yandex-team.ru/AntiAdblock/partners_money2, при необходимости можно создать новый
```python
import statface_client

client = statface_client.StatfaceClient(host=statface_client.STATFACE_BETA, oauth_token='__token_placeholder__')  # for production host=statface_client.STATFACE_PRODUCTION
report = client.get_report('AntiAdblock/partners_money3')
new_config = statface_client.StatfaceReportConfig(
    title='Partners money v3',
    dimensions=[{'fielddate': 'date'},
                {'service_id': 'tree'}],
    measures=[{'aab_money': 'number'},
              {'money': 'number'},
              {'aab_shows': 'number'},
              {'shows': 'number'},
              {'aab_clicks': 'number'},
              {'clicks': 'number'},
              {'aab_fraud_shows': 'number'},
              {'aab_fraud_clicks': 'number'}],
    titles={
        'aab_money': u'Деньги (адб)',
        'money': u'Деньги',
        'aab_shows': u'Количество показов (адб)',
        'shows': u'Количество показов',
        'aab_clicks': u'Количество кликов (адб)',
        'clicks': u'Количество кликов',
        'aab_fraud_shows': u'Количество фродовых показов (адб)',
        'aab_fraud_clicks': u'Количество фродовых кликов (адб)',
        'unblock': u'Процент разблокированных денег',
        'frauded_shows_percent': u'Процент фродовых показов (адб)',
        'frauded_clicks_percent': u'Процент фродовых кликов (адб)',
        'ctr_adb': u'CTR (адб)',
        'ctr': u'CTR',
    },
    hidden={'aab_shows': 1,
            'shows': 1,
            'aab_clicks': 1,
            'clicks': 1,
            'aab_fraud_shows': 1,
            'aab_fraud_clicks': 1,
            'frauded_shows_percent': 1,
            'frauded_clicks_percent': 1},
    graphs=[
        {'title': 'Деньги',
         'fields': ['aab_money', 'money'],
         'colors': {'aab_money': '4F81BC', 'money': 'BF0000'},
         'type': 'line'},
        {'title': 'Процент разблокированных денег',
         'fields': ['unblock'],
         'type': 'area'},
        {'title': 'Процент фрода',
         'fields': ['frauded_shows_percent', 'frauded_clicks_percent'],
         'type': 'line'},
        {'title': 'CTR',
         'fields': ['ctr_adb', 'ctr'],
         'type': 'line'},
    ],
    calculations=[
        {'unblock': {
            'after': 'money',
            'default': 0,
            'expression': '100*(aab_money/money)'
        }},
        {'frauded_shows_percent': {
            'after': 'aab_fraud_shows',
            'default': 0,
            'expression': '100*(aab_fraud_shows/aab_shows)'
        }},
        {'frauded_clicks_percent': {
            'after': 'aab_fraud_clicks',
            'default': 0,
            'expression': '100*(aab_fraud_clicks/aab_clicks)'
        }},
        {'ctr_adb': {
            'after': 'aab_clicks',
            'default': 0,
            'expression': '100*((aab_clicks-aab_fraud_clicks)/aab_shows)'
        }},
        {'ctr': {
            'after': 'clicks',
            'default': 0,
            'expression': '100*(clicks/shows)'
        }},
    ],
    period_distance=60,
    region='RU',
)
report.upload_config(new_config)
```

## Переключение расчетов на другой YT кластер
Расчеты можно выполнять как на Hahn, так и Arnold. В случае даунтайма одного из кластеров, 
нужно выполнить Sandbox-таску [ANTIADBLOCK_SAVE_AVAILABLE_YT_CLUSTERS](https://sandbox.yandex-team.ru/tasks?type=ANTIADBLOCK_SAVE_AVAILABLE_YT_CLUSTERS), 
В настройках таски в разделе `YT cluster settings` нужно выбрать доступные кластеры. 
Таска сохранит ресурс типа [ANTIADBLOCK_AVAILABLE_YT_CLUSTERS_RESOURCE](https://sandbox.yandex-team.ru/resources?type=ANTIADBLOCK_AVAILABLE_YT_CLUSTERS_RESOURCE)   
Пример содержимого ресурса:
```json
["arnold", "hahn"]
```
