# coding: utf-8

from itertools import product

from juggler_sdk import Check, Child, FlapOptions

from antiadblock.tasks.tools.const import VALUABLE_SERVICES
from antiadblock.tasks.tools.notifications import DAY_PHONE, TELEGRAM
from antiadblock.tasks.tools.juggler import create_aggregate_checks, JUGGLER_MONEY_HOST


MONEY_RAW_EVENTS_SERVICE_TEMPLATE = 'aab_report_{}_{}_{}'

SERVICE_CHECK_TYPES = ['money_drop', 'no_data', 'negative_trend']


def run(services):

    money_checks = list()
    report_update_check = Check(
        host=JUGGLER_MONEY_HOST, service='aab_report_update', namespace='antiadb',
        ttl=0, refresh_time=60, tags=['report_update'], mark='antiadb-money', notifications=[TELEGRAM]
    )
    total_no_data = Check(
        host=JUGGLER_MONEY_HOST, service='aab_total_no_data', namespace='antiadb',
        ttl=0, refresh_time=60, tags=['report_update'], mark='antiadb-money', notifications=[TELEGRAM, DAY_PHONE],
        pronounce='На всех сервисах нет данных по деньгам'
    )
    total_money_drop = Check(
        host=JUGGLER_MONEY_HOST, service='aab_total_money_drop', namespace='antiadb',
        ttl=0, refresh_time=60, tags=['report_update'], mark='antiadb-money', notifications=[TELEGRAM, DAY_PHONE],
        pronounce='На всех сервисах резкое падение денег'
    )
    money_checks += ([report_update_check, total_no_data, total_money_drop])

    for check_type, service_id in product(SERVICE_CHECK_TYPES, services.keys()):
        for device in services[service_id]:
            # Заводим агрегаты на события money_drop, negative_trend, no_data по service_id по сырым событиям
            money_check = Check(
                host=JUGGLER_MONEY_HOST,
                service=MONEY_RAW_EVENTS_SERVICE_TEMPLATE.format(check_type, service_id, device),
                namespace='antiadb',
                ttl=0, refresh_time=60,
                tags=[check_type], mark='antiadb-money',
                notifications=[TELEGRAM],
            )
            money_checks.append(money_check)

            if check_type == 'money_drop' and service_id in VALUABLE_SERVICES and device == 'desktop':
                # Для money_drop на важных сервисах также заводим звонок c флаподавом
                money_check_with_call = Check(
                    host='antiadb_money_call', service=service_id, namespace='antiadb',
                    aggregator='logic_or', aggregator_kwargs=dict(nodata_mode='skip'),
                    ttl=0, refresh_time=60,
                    tags=[check_type], mark='antiadb-money',
                    notifications=[DAY_PHONE], flaps_config=FlapOptions(stable=900, critical=1800),
                    pronounce='На сервисе {} резкое падение денег'.format(service_id.replace('_', ' ').lstrip('.').lstrip()),
                    children=[Child(host=JUGGLER_MONEY_HOST,
                                    service=MONEY_RAW_EVENTS_SERVICE_TEMPLATE.format(check_type, service_id, device))]
                )
                money_checks.append(money_check_with_call)

    create_aggregate_checks(money_checks, mark="antiadb-money")
