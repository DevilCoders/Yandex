"""
Metric API for service status visualization
TODO: https://st.yandex-team.ru/ANTIADB-1320. Take metrics from solomon instead of elastic

"""
from flask import Blueprint, jsonify, request, g, current_app

from antiadblock.configs_api.lib.context import CONTEXT
from antiadblock.configs_api.lib.auth.permissions import ensure_permissions_on_service
from antiadblock.configs_api.lib.auth.permission_models import PermissionKind
from antiadblock.configs_api.lib.metrics.helpers import percentage, date_params_from_range
from antiadblock.configs_api.lib.jsonlogging import logging_context

CRYPROX_ES_INDEX = '*:*antiadb-cryprox-*'
NGINX_ES_INDEX = '*:*antiadb-nginx-*'


metrics_api = Blueprint('metrics_api', __name__)

MAX_DATE_RANGE = 7 * 24 * 60  # Max date range to select from elastic search. Current - 7 days
INVALID_RANGE_MSG_TEMPLATE = 'Invalid date range interval: {invalid_range}. Allowed range is (15, %s).' % MAX_DATE_RANGE
INVALID_PERCENTILE_VAL_MSG_TEMPLATE = 'Invalid percent arg values: {percentile}. Must be integer in range (1, 99).'
INVALID_METRIC_NAME_MSG_TEMPLATE = 'Metric with name `{metric_name}` not found.'

# Adblock aps to show within partners graphs
SHOWN_ADB_APPS = ['ADBLOCK', 'ADGUARD', 'GHOSTERY', 'UBLOCK', 'UNKNOWN']


@metrics_api.route('/metrics/<string:service_id>/<string:metric_handler_name>')
def get_metrics(service_id, metric_handler_name):
    ensure_permissions_on_service(service_id, PermissionKind.SERVICE_SEE)
    g.service_id = service_id
    with logging_context(service_id=service_id):
        g.metric_handler_name = metric_handler_name
        date_range = int(request.args.get('date_range', 0))
        if not 15 <= date_range <= MAX_DATE_RANGE:
            return jsonify({'message': INVALID_RANGE_MSG_TEMPLATE.format(invalid_range=date_range),
                            'request_id': g.request_id}), 400

        from_date_ts, group_by_time = date_params_from_range(date_range)
        handler = handler_by_metric_name(metric_handler_name)

        return handler(date_from=from_date_ts, histogram_group_interval=group_by_time)


def handler_by_metric_name(handler_name):
    """
    Find handler by metric name from uri path
    :return: handler function
    """
    metric_handlers = {'http_codes': simple_http_codes_handler,
                       'http_errors_domain_type': http_errors_by_domain_type_handler,
                       'bamboozled_by_app': bamboozled_by_app_handler,
                       'bamboozled_by_bro': bamboozled_by_bro_handler,
                       'fetch_timings': percentiles_handler,
                       'adblock_apps_proportions': adblock_apps_proportions_handler}

    return metric_handlers.get(handler_name, unknown_metric_handler)


def unknown_metric_handler(*a, **kw):
    msg = INVALID_METRIC_NAME_MSG_TEMPLATE.format(metric_name=g.metric_handler_name)
    current_app.logger.warning(msg)
    return jsonify({'message': msg, 'request_id': g.request_id}), 404


def simple_http_codes_handler(date_from, histogram_group_interval):
    """
    :param date_from: start date for metric search in milliseconds timestamp format
    :param histogram_group_interval: time interval in minutes to group by metrics
    :return: dict with http codes for service_id grouped by time and codes type
    {"1519127280000": {"304": 225, "404": 172, "502": 5, "200": 75521, "301": 3, "302": 3743},...}
    """
    return jsonify(CONTEXT.metrics.date_histogram_splitted_by_one_field(date_gt=date_from,
                                                                        histogram_interval=histogram_group_interval,
                                                                        service_id=g.service_id))


def http_errors_by_domain_type_handler(date_from, histogram_group_interval):
    """
    :param date_from: start date for metric search in milliseconds timestamp format
    :param histogram_group_interval: time interval in minutes to group by metrics
    :return: dict with http errors count for service_id grouped by time and url tag (YANDEX, PARTNER, etc)
    {1521706500000: {u'PARTNER': 8, u'PCODE': 1, u'YANDEX': 1}, 1521706560000: {u'PARTNER': 157, u'PCODE': 35, u'YANDEX': 43, u'BK': 10},...}
    """
    allowed_tags_mapping = {'YANDEX': 'ADS', 'PARTNER': 'PARTNER', 'WIDGETS': 'WIDGETS'}

    result = CONTEXT.metrics.date_histogram_splitted_by_one_field(date_gt=date_from,
                                                                  histogram_interval=histogram_group_interval,
                                                                  service_id=g.service_id)

    final_result = dict()
    for ts, data in result.items():
        final_result[ts] = {allowed_tags_mapping[k]: v for k, v in data.items() if k in allowed_tags_mapping}

    return jsonify(final_result)


def bamboozled_by_bro_handler(date_from, **kw):
    """
    Calculate bamboozled percentage grouped by 10 minutes and browser name from shown_browsers_map
    :param date_from: start date for metric search in milliseconds timestamp format
    :return: dict with bamboozled percentage grouped by 10 minutes and browser name:
    {1518912000000: {u'Opera': 96, u'Safari': 60, u'Chrome': 86, u'Firefox': 92, u'Yandex Browser': 91, u'Edge': 66, u'Internet Explorer': 3}, ...}
    """

    shown_browsers_map = {'opera': 'Opera',
                          'safari': 'Safari',
                          'chrome': 'Chrome',
                          'firefox': 'Firefox',
                          'yandex_browser': 'Yandex Browser',
                          'edge': 'Edge',
                          'ie': 'Internet Explorer'}

    bamboozled_events = CONTEXT.metrics.date_histogram_splitted_by_two_fields(date_gt=date_from,
                                                                              histogram_interval='30m',
                                                                              service_id=g.service_id)

    result = dict()
    for ts, adb_browser_events in bamboozled_events.items():
        for browser, adb_bamboozle_events in adb_browser_events.items():
            browser = shown_browsers_map.get(browser, None)
            if browser is not None:
                result.setdefault(ts, dict())
                try_to_render = adb_bamboozle_events.get('try_to_render', 0)
                confirmed = adb_bamboozle_events.get('confirm_block', 0)
                result[ts].update({browser: percentage(part=confirmed, whole=try_to_render)})

    return jsonify(result)


def bamboozled_by_app_handler(date_from, **kw):
    """
    Calculate bamboozled percentage grouped by 10 minutes and adblock app
    :param date_from: start date for metric search in milliseconds timestamp format
    :return: dict with bamboozled percentage grouped by 10 minutes and adblock app:
    {1518912000000: {u'ADBLOCK': 96, u'GHOSTERY': 60, u'UBLOCK': 86, u'NOT_BLOCKED': 92, u'ADBLOCKPLUS': 91, u'UNKNOWN': 66, u'ADGUARD': 3}, ...}
    """
    bamboozled_events = CONTEXT.metrics.date_histogram_splitted_by_two_fields(date_gt=date_from,
                                                                              histogram_interval='30m',
                                                                              service_id=g.service_id)

    result = dict()
    for ts, adb_app_events in bamboozled_events.items():
        result.update({ts: dict()})
        for adb_app, adb_bamboozle_events in adb_app_events.items():
            try_to_render = adb_bamboozle_events.get('try_to_render', 0)
            confirmed = adb_bamboozle_events.get('confirm_block', 0)
            result[ts].update({adb_app: percentage(part=confirmed, whole=try_to_render)})

    return jsonify(result)


def percentiles_handler(date_from, histogram_group_interval):
    """
    :param date_from: start date for metric search in milliseconds timestamp format
    :param histogram_group_interval: time interval in minutes to group by metrics
    :return: dict with http codes for service_id grouped by time and codes type
    {"1519127280000": {"304": 225, "404": 172, "502": 5, "200": 75521, "301": 3, "302": 3743},...}
    """
    percentiles = request.args.getlist('percentile')
    if len(percentiles) == 0:
        percentiles = None

    def percents_validation(percents):
        """
        Simple percent arg validator
        :param percents: list with percents - [u'95', u'98']
        """
        for percent in percents:
            try:
                if int(percent) not in xrange(1, 100):
                    return False
            except:
                return False
        return True

    if percentiles is None or percents_validation(percentiles) is False:
        return jsonify({'message': INVALID_PERCENTILE_VAL_MSG_TEMPLATE.format(percentile=percentiles),
                        'request_id': g.request_id}), 400

    return jsonify(CONTEXT.metrics.date_histogram_action_timings_percentiles(service_id=g.service_id,
                                                                             action='fetch_content',
                                                                             date_gt=date_from,
                                                                             histogram_interval=histogram_group_interval,
                                                                             percents=percentiles))


def adblock_apps_proportions_handler(date_from, histogram_group_interval):
    """
    :param date_from: start date for metric search in milliseconds timestamp format
    :param histogram_group_interval: time interval in minutes to group by metrics
    :return: dict with bamboozled try_to_render event counts for service_id grouped by time and adblock app
    {1521706500000: {u'ADBLOCK': 8, u'ADGUARD': 1, u'UBLOCK': 1}, 1521706560000: {u'ADBLOCKPLUS': 157, u'UNKNOWN': 35, u'ADGUARD': 43, u'UBLOCK': 10},...}
    """

    return jsonify(CONTEXT.metrics.date_histogram_splitted_by_one_field(date_gt=date_from,
                                                                        histogram_interval=histogram_group_interval,
                                                                        service_id=g.service_id))
