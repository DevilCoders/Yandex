import pytest
import pandas as pd

from antiadblock.tasks.tools.yt_utils import get_available_yt_cluster
import antiadblock.tasks.tools.common_configs as configs
from antiadblock.tasks.money_by_service_id.lib.lib import Columns, DIMENSIONS, AGGREGATION_FIELDS, PREFIXES, METRICS, get_dataframe, consume_dataframe, get_sensors


BSDSP_YQL = pd.DataFrame(
    data=[
        ['2019-12-10 00:00:00', 'autoru'] + ['direct', 'desktop', 'auto.ru'] + [0, 0] + [1] * 3,
        ['2019-12-10 00:00:00', 'autoru'] + ['direct', 'desktop', 'auto.ru'] + [1, 0] + [1] * 3,
        ['2019-12-10 00:00:00', 'autoru'] + ['direct', 'desktop', 'auto.ru'] + [0, 1] + [1] * 3,
        ['2019-12-10 00:00:00', 'autoru'] + ['direct', 'desktop', 'auto.ru'] + [1, 1] + [1] * 3,
        ['2019-12-10 01:00:00', 'autoru'] + ['direct', 'desktop', 'auto.ru'] + [0, 0] + [1] * 3,
        ['2019-12-10 01:00:00', 'autoru'] + ['direct', 'desktop', 'auto.ru'] + [1, 0] + [1] * 3,
        ['2019-12-10 01:00:00', 'autoru'] + ['direct', 'desktop', 'auto.ru'] + [0, 1] + [1] * 3,
        ['2019-12-10 01:00:00', 'autoru'] + ['direct', 'desktop', 'auto.ru'] + [1, 1] + [1] * 3,
    ],
    columns=DIMENSIONS + AGGREGATION_FIELDS + PREFIXES + METRICS,
)
BSCHEVENT_ONLY_BLOCKS_YQL = pd.DataFrame(
    data=[
        ['2019-12-10 01:00:00', 'autoru'] + ['direct', 'desktop', 'auto.ru'] + [0, 0] + [2] * 3,
        ['2019-12-10 01:00:00', 'autoru'] + ['direct', 'desktop', 'auto.ru'] + [1, 0] + [2] * 3,
        ['2019-12-10 01:00:00', 'autoru'] + ['direct', 'desktop', 'auto.ru'] + [0, 1] + [2] * 3,
        ['2019-12-10 01:00:00', 'autoru'] + ['direct', 'desktop', 'auto.ru'] + [1, 1] + [2] * 3,
        ['2019-12-10 01:00:00', 'docviewer'] + ['direct', 'desktop', 'docviewer.yandex.ru'] + [0, 0] + [1] * 3,
        ['2019-12-10 01:00:00', 'docviewer'] + ['direct', 'desktop', 'docviewer.yandex.ru'] + [1, 0] + [1] * 3,
        ['2019-12-10 01:00:00', 'docviewer'] + ['direct', 'desktop', 'docviewer.yandex.ru'] + [0, 1] + [1] * 3,
        ['2019-12-10 01:00:00', 'docviewer'] + ['direct', 'desktop', 'docviewer.yandex.ru'] + [1, 1] + [1] * 3,
    ],
    columns=DIMENSIONS + AGGREGATION_FIELDS + PREFIXES + METRICS,
)
EVENTBAD_YQL = pd.DataFrame(
    data=[
        ['2019-12-10 00:00:00', 'autoru'] + ['direct', 'desktop', 'auto.ru'] + [0] + [1] * 3,
        ['2019-12-10 00:00:00', 'autoru'] + ['direct', 'desktop', 'auto.ru'] + [1] + [1] * 3,
        ['2019-12-10 01:00:00', 'autoru'] + ['direct', 'desktop', 'auto.ru'] + [0] + [1] * 3,
        ['2019-12-10 01:00:00', 'autoru'] + ['direct', 'desktop', 'auto.ru'] + [1] + [1] * 3,
        ['2019-12-10 01:00:00', 'docviewer'] + ['direct', 'desktop', 'docviewer.yandex.ru'] + [0] + [1] * 3,
        ['2019-12-10 01:00:00', 'docviewer'] + ['direct', 'desktop', 'docviewer.yandex.ru'] + [1] + [1] * 3,
    ],
    columns=DIMENSIONS + AGGREGATION_FIELDS + [Columns.aab.name] + METRICS,
)
EXPECTED_JOINED = pd.DataFrame(
    data=[
        [pd.Timestamp('2019-12-10 00:00:00'), 'autoru', 'direct', 'desktop', 'auto.ru', 2, 1, 4, 4, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1],
        [pd.Timestamp('2019-12-10 01:00:00'), 'autoru', 'direct', 'desktop', 'auto.ru', 4, 2, 8, 8, 4, 4, 4, 4, 4, 2, 2, 2, 1, 1, 1, 1, 1, 1],
        [pd.Timestamp('2019-12-10 01:00:00'), 'docviewer', 'direct', 'desktop', 'docviewer.yandex.ru', 2, 1, 4, 4, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1],
    ],
    columns=DIMENSIONS + AGGREGATION_FIELDS + ['money', 'aab_money', 'shows', 'clicks', 'aab_shows', 'aab_clicks', 'fraud_money', 'fraud_shows',
                                               'fraud_clicks', 'aab_fraud_money', 'aab_fraud_shows', 'aab_fraud_clicks', 'bad_money',
                                               'aab_bad_money', 'bad_shows', 'aab_bad_shows', 'bad_clicks', 'aab_bad_clicks']
).set_index(DIMENSIONS + AGGREGATION_FIELDS)


def test_get_dataframe():
    dataframe = get_dataframe(BSDSP_YQL, BSCHEVENT_ONLY_BLOCKS_YQL, EVENTBAD_YQL)
    assert dataframe.equals(EXPECTED_JOINED)

    joined_spoiled = EXPECTED_JOINED.copy()
    joined_spoiled.iloc[0]['money'] = 0
    assert not dataframe.equals(joined_spoiled)


@pytest.mark.parametrize('scale', configs.Scales)
def test_consume_dataframe_stat(scale):
    dataframe = EXPECTED_JOINED.xs(pd.Timestamp('2019-12-10 00:00:00'), level=Columns.date.name, drop_level=False)  # left only one row
    expeted_stat_row = {
        'service_id': '---placeholder---',
        'fielddate': '2019-12-10 00:00:00',
        'money': 2,
        'aab_money': 1,
        'shows': 4,
        'clicks': 4,
        'aab_shows': 2,
        'aab_clicks': 2,
        'fraud_money': 2,
        'fraud_shows': 2,
        'fraud_clicks': 2,
        'aab_fraud_money': 1,
        'aab_fraud_shows': 1,
        'aab_fraud_clicks': 1,
        'bad_money': 1,
        'aab_bad_money': 1,
        'bad_shows': 1,
        'aab_bad_shows': 1,
        'bad_clicks': 1,
        'aab_bad_clicks': 1
    }
    expected_stat_data = []
    for tree in [['TOTAL'],
                 ['TOTAL', 'producttype', 'direct'],
                 ['TOTAL', 'device', 'desktop'],
                 ['TOTAL', 'domain', 'auto.ru'],
                 ['TOTAL', 'producttype', 'direct', 'device', 'desktop'],
                 ['autoru'],
                 ['autoru', 'producttype', 'direct'],
                 ['autoru', 'device', 'desktop'],
                 ['autoru', 'domain', 'auto.ru'],
                 ['autoru', 'producttype', 'direct', 'device', 'desktop']]:
        expected_stat_data.append(expeted_stat_row.copy())
        expected_stat_data[-1]['service_id'] = tree

    stat, _ = consume_dataframe(dataframe, scale=scale)
    assert stat == expected_stat_data


@pytest.mark.parametrize('scale', configs.Scales)
def test_consume_dataframe_solomon(scale):
    dataframe = EXPECTED_JOINED.xs(pd.Timestamp('2019-12-10 00:00:00'), level=Columns.date.name, drop_level=False)  # left only one row
    expeted_sensors = [
        {'ts': 1575925200.0, 'value': 50.0, 'labels': {'sensor': 'ratio name', 'service_id': 'autoru', 'producttype': '_all', 'device': '_all'}},
        {'ts': 1575925200.0, 'value': 50.0, 'labels': {'sensor': 'ratio name', 'service_id': 'autoru', 'device': '_all', 'producttype': 'direct'}},
        {'ts': 1575925200.0, 'value': 50.0, 'labels': {'sensor': 'ratio name', 'service_id': 'autoru', 'producttype': '_all', 'device': 'desktop'}},
    ]
    _, solomon = consume_dataframe(dataframe, scale=scale, calculate_ratios=[('ratio name', ('aab_money', 'money'))])
    assert solomon == expeted_sensors


@pytest.mark.parametrize('ratio, value', [
    [[('name', ('aab_money', 'money'))], 50.],
    [[('name', ('money', 'money'))], 100.],
    [[('name', ('money', 'bad_money'))], 200.],
])
def test_get_sensors(ratio, value):
    dataframe = EXPECTED_JOINED.xs(pd.Timestamp('2019-12-10 00:00:00'), level=Columns.date.name, drop_level=False)  # left only one row
    solomon = get_sensors(dataframe, ratios=ratio)
    assert solomon == [{'ts': 1575925200.0, 'value': value, 'labels': {'sensor': 'name', 'service_id': 'autoru'}}]


@pytest.mark.parametrize('current_cluster,available_clusters,expected_cluster,is_exception', [
    ['hahn', '["hahn", "arnold"]', 'hahn', False],
    ['hahn', '["arnold"]', 'arnold', False],
    ['hahn', None, 'hahn', False],
    ['hahn', '[]', 'hahn', True],
])
def test_get_available_yt_cluster(current_cluster, available_clusters, expected_cluster, is_exception):
    if not is_exception:
        assert expected_cluster == get_available_yt_cluster(current_cluster, available_clusters)
    else:
        with pytest.raises(Exception):
            get_available_yt_cluster(current_cluster, available_clusters)
