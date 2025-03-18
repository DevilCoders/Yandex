# coding=utf-8
import os
import sys
from datetime import timedelta, datetime

import requests
from yql.api.v1.client import YqlClient

from antiadblock.tasks.tools.common_configs import YT_CLUSTER
from antiadblock.tasks.tools.yt_utils import get_available_yt_cluster

query_template = '''
PRAGMA AnsiInForEmptyOrNullableItemsCollections;
PRAGMA yt.Pool="antiadb";
$log = "logs/bs-chevent-log/1d";
$table_from = '{date_min}';
$table_to = '{date_max}';
$partner_pageids_table = '//home/antiadb/monitorings/antiadb_partners_pageids/{date_max}';
$service_id = '{service_id}';

$partner_pageids = (select pageid from $partner_pageids_table where service_id=$service_id);

$is_aab = ($adbbits) -> {{return $adbbits is not NULL and $adbbits != '0';}};
SELECT
    t.*,
    (100.0 * t.aab_money / t.money) as money_percent,
    (100.0 * (t.aab_shows-t.aab_fraud_shows) / (t.shows-t.fraud_shows)) as shows_percent,
    (100.0 * (t.aab_clicks-t.aab_fraud_clicks) / (t.clicks-t.fraud_clicks)) as clicks_percent,
    (100.0 * (t.aab_clicks-aab_fraud_clicks) / (t.aab_shows-aab_fraud_shows)) as aab_ctr,
    (100.0 * (t.clicks-t.fraud_clicks) / (t.shows-t.fraud_shows)) as ctr,
    (100.0 * t.aab_fraud_shows / t.aab_shows) as aab_shows_fraud_percent,
    (100.0 * t.aab_fraud_clicks / t.aab_clicks) as aab_clicks_fraud_percent,
    (100.0 * t.fraud_shows / t.shows) as shows_fraud_percent,
    (100.0 * t.fraud_clicks / t.clicks) as clicks_fraud_percent,
from (
    SELECT
      time, pageid,
          sum(if(fraudbits='0', cast(eventcost as uint64), 0)) * 30 / 1000000 / 1.18 as money,
          sum(if(fraudbits='0' and $is_aab(adbbits), cast(eventcost as uint64), 0)) * 30 / 1000000 / 1.18 as aab_money,
          count_if(countertype='1' and $is_aab(adbbits)) as aab_shows,
          count_if(countertype='1') as shows,
          count_if(countertype='2' and $is_aab(adbbits)) as aab_clicks,
          count_if(countertype='2') as clicks,
          count_if(countertype='1' and fraudbits!='0' and $is_aab(adbbits)) as aab_fraud_shows,
          count_if(countertype='2' and fraudbits!='0' and $is_aab(adbbits)) as aab_fraud_clicks,
          count_if(countertype='1' and fraudbits!='0') as fraud_shows,
          count_if(countertype='2' and fraudbits!='0') as fraud_clicks
    FROM
      RANGE($log, $table_from, $table_to) as log
    WHERE
      log.placeid in ('542', '1542') and pageid in $partner_pageids
    group by SUBSTRING(log.iso_eventtime, 0, 10) as time, pageid
) as t
ORDER BY t.time;
'''

query_fillrate_template = '''
PRAGMA AnsiInForEmptyOrNullableItemsCollections;
PRAGMA yt.Pool="antiadb";
$log = "logs/bs-dsp-log/1d";
$table_from = '{date_min}';
$table_to = '{date_max}';
$partner_pageids_table = '//home/antiadb/monitorings/antiadb_partners_pageids/{date_max}';
$service_id = '{service_id}';

$partner_pageids = (select pageid from $partner_pageids_table where service_id=$service_id);

$is_aab = ($adbbits) -> {{return $adbbits is not NULL and $adbbits != '0';}};
$preselect = (
    select
      adbbits, iso_eventtime, dspid, pageid
    FROM
      RANGE($log, $table_from, $table_to)
    WHERE
      countertype='0' and position='0' and dspfraudbits='0' and dspeventflags='0' and win='1' and
      pageid in $partner_pageids
);

SELECT
    t.*,
    (100.0 * t.wins / t.hits) as fill_rate,
    (100.0 * t.not_aab_wins / t.not_aab_hits) as not_aab_fill_rate,
    (100.0 * t.aab_wins / t.aab_hits) as aab_fill_rate,
    (100.0 * t.aab_hits / t.hits) as hits_percent,
    (100.0 * t.aab_wins / t.wins) as wins_percent,
from (
    SELECT
        time, pageid,
        count(*) as hits,
        count_if(not $is_aab(adbbits)) as not_aab_hits,
        count_if($is_aab(adbbits)) as aab_hits,
        count_if(dspid='1') as wins,
        count_if(dspid='1' and not $is_aab(adbbits)) as not_aab_wins,
        count_if(dspid='1' and $is_aab(adbbits)) as aab_wins,
    FROM $preselect
    group by SUBSTRING(iso_eventtime, 0, 10) as time, pageid
) as t
ORDER BY time;
'''


def get_yql_access_list():
    url = 'https://abc-back.yandex-team.ru/api/v4/services/members/?fields=person.login&is_robot=false&service=1526'
    token = os.getenv('ABC_TOKEN', '')
    try:
        resp = requests.get(url, headers={'Authorization': 'OAuth {}'.format(token)}).json()
        persons = set()
        for p in resp['results']:
            persons.add(p['person']['login'])
        return list(persons)
    except Exception as e:
        sys.stderr.write('Failed to get access list from abc, falling back to default users list, error: ' + str(e))
        return ['vsalavatov', 'dridgerve', 'tzapil', 'evor', 'aydarboss', 'alex-kuz']


def get_statistics_by_partner_pageid(service_id, date_start):
    yt_token = os.getenv('YT_TOKEN')
    assert yt_token is not None
    yt_cluster = get_available_yt_cluster(YT_CLUSTER, os.getenv('AVAILABLE_YT_CLUSTERS'))
    yql_client = YqlClient(token=yt_token, db=yt_cluster)

    date_min = (date_start - timedelta(weeks=2)).strftime("%Y-%m-%d")
    date_max = datetime.now().strftime("%Y-%m-%d")

    query = query_template.format(service_id=service_id, date_min=date_min, date_max=date_max)

    yql_request = yql_client.query(query, syntax_version=1, title='STATISTICS_BY_PARTNER_PAGEIDS {} YQL'.format(os.path.basename(__file__)))
    yql_response = yql_request.run(share_with=get_yql_access_list())

    query_fillrate = query_fillrate_template.format(service_id=service_id, date_min=date_min, date_max=date_max)

    yql_request_fillrate = yql_client.query(query_fillrate, syntax_version=1, title='FILLRATE_BY_PARTNER_PAGEIDS {} YQL'.format(os.path.basename(__file__)))
    yql_response_fillrate = yql_request_fillrate.run(share_with=get_yql_access_list())

    return '{}\n{}'.format(yql_response.share_url, yql_response_fillrate.share_url)
