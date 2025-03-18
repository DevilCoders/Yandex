import pytest

from antiadblock.configs_api.lib.metrics import metrics_api


@pytest.mark.skip(reason="https://st.yandex-team.ru/ANTIADB-1320. Take metrics from solomon instead of elastic")
@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize('path, qargs, expected_code, expected_answer',
                         [('metrics/yandex_news/fetch', {'date_range': 15, 'percentile': 95}, 404, metrics_api.INVALID_METRIC_NAME_MSG_TEMPLATE.format(metric_name='fetch')),
                          ('metrics/yandex_news/fetch_timings', {'date_range': 5, 'percentile': 95}, 400, metrics_api.INVALID_RANGE_MSG_TEMPLATE.format(invalid_range=5)),
                          ('metrics/fetch_timings', {'date_range': 5, 'percentile': 95}, 404, 'NotFound'),
                          ('metrics/yandex_news/', {'date_range': 5, 'percentile': 95}, 404, 'NotFound'),
                          ('metrics/yandex_news/fetch_timings', {'date_range': 60, 'percentile': 101}, 400, metrics_api.INVALID_PERCENTILE_VAL_MSG_TEMPLATE.format(percentile=[u'101'])),
                          ('metrics/yandex_news/fetch_timings', {'date_range': 60}, 400, metrics_api.INVALID_PERCENTILE_VAL_MSG_TEMPLATE.format(percentile=None))])
def test_wrong_requests(api, session, path, qargs, expected_code, expected_answer):

    response = session.get(api[path], params=qargs)
    response_text = response.json()

    assert response_text['message'] == expected_answer
    assert response.status_code == expected_code


@pytest.mark.skip(reason="https://st.yandex-team.ru/ANTIADB-1320. Take metrics from solomon instead of elastic")
@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize('path, qargs, expected_answer',
                         [('metrics/yandex_news/fetch_timings', {'date_range': 15, 'percentile': 95},
                           {u'1522157820000': {u'95': 811}, u'1522157100000': {u'95': 831}, u'1522156980000': {u'95': 818}, u'1522157040000': {u'95': 844}, u'1522156920000': {u'95': 859}}),
                          ('metrics/yandex_news/http_codes', {'date_range': 15},
                           {u'1522157820000': {u'200': 25240, u'599': 2, u'301': 1, u'302': 977, u'304': 114, u'404': 40, u'504': 1},
                            u'1522157340000': {u'200': 69199, u'599': 2, u'301': 1, u'302': 2628, u'304': 293, u'404': 122, u'403': 3, u'502': 3},
                            u'1522156920000': {u'200': 48293, u'599': 2, u'302': 1934, u'304': 161, u'404': 89, u'504': 1, u'502': 1}}),
                          ('metrics/yandex_news/bamboozled_by_app', {'date_range': 15},
                           {u'1522243800000': {u'UBLOCK': 86, u'ADBLOCK': 86, u'GHOSTERY': 79, u'UNKNOWN': 83, u'ADGUARD': 78}}),
                          ('metrics/yandex_news/bamboozled_by_bro', {'date_range': 15},
                           {u'1522157400000': {u'Firefox': 89, u'Yandex Browser': 91, u'Chrome': 91, u'Opera': 90, u'Edge': 96, u'Safari': 84, u'Internet Explorer': 90},
                            u'1522155600000': {u'Firefox': 89, u'Yandex Browser': 91, u'Chrome': 91, u'Opera': 90, u'Edge': 85, u'Safari': 82, u'Internet Explorer': 91}}),
                          ('metrics/yandex_news/http_errors_domain_type', {'date_range': 15},
                           {u'1522157820000': {u'PARTNER': 43, u'ADS': 2}, u'1522157700000': {u'PARTNER': 125, u'ADS': 1}, u'1522157760000': {u'PARTNER': 116, u'ADS': 21},
                            u'1522156920000': {u'PARTNER': 86, u'ADS': 5}}),
                          ('metrics/yandex_news/adblock_apps_proportions', {'date_range': 15},
                           {u'1523545440000': {u'ADGUARD': 80, u'GHOSTERY': 7, u'UBLOCK': 147, u'UNKNOWN': 409, u'ADBLOCK': 649},
                            u'1523544600000': {u'ADGUARD': 86, u'GHOSTERY': 1, u'UBLOCK': 143, u'UNKNOWN': 391, u'ADBLOCK': 713},
                            u'1523544540000': {u'ADGUARD': 14, u'UBLOCK': 20, u'UNKNOWN': 66, u'ADBLOCK': 76}}),
                          ])
def test_basic_metrics(api, session, elasticsearch_stub_server, path, qargs, expected_answer):
    response = session.get(api[path], params=qargs)
    response_text = response.json()

    assert response_text == expected_answer
    assert response.status_code == 200


@pytest.mark.skip(reason="https://st.yandex-team.ru/ANTIADB-1320. Take metrics from solomon instead of elastic")
@pytest.mark.parametrize('metric_name, expected_handler',
                         [('http_codes', metrics_api.simple_http_codes_handler),
                          ('http_errors_domain_type', metrics_api.http_errors_by_domain_type_handler),
                          ('bamboozled_by_app', metrics_api.bamboozled_by_app_handler),
                          ('bamboozled_by_bro', metrics_api.bamboozled_by_bro_handler),
                          ('fetch_timings', metrics_api.percentiles_handler),
                          ('adblock_apps_proportions', metrics_api.adblock_apps_proportions_handler),
                          ('fetch_timer', metrics_api.unknown_metric_handler),
                          ('http_code', metrics_api.unknown_metric_handler),
                          ('', metrics_api.unknown_metric_handler)])
def test_handler_by_metric_name(metric_name, expected_handler):
    assert metrics_api.handler_by_metric_name(metric_name) is expected_handler
