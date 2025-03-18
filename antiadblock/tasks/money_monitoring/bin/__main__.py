# -*- coding: utf-8 -*-
import os
from collections import OrderedDict, defaultdict

from tvmauth import TvmClient, TvmApiClientSettings

from antiadblock.tasks.tools.juggler import send_event
from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.const import CLAIM_LINK_TEMPLATE
from antiadblock.tasks.tools.stat import StatReports, VALUABLE_IDS
from antiadblock.tasks.tools.misc import get_datetime_for_incident_start
from antiadblock.tasks.tools.configs_api import post_tvm_request, get_monitoring_settings
from antiadblock.tasks.money_monitoring.lib.data_checks import check_money_drop, check_no_data, check_negative_trend, ACCEPTABLE_DATA_DELAY

TICKET_CREATED = 201
TICKET_CHANGED = 205
TICKET_ALREADY_EXISTS = 208

FAILED_CHECKS = defaultdict(int)

JUGGLER_MONEY_HOST = 'antiadb_money'


SOLOMON_GRAPH_TEMPLATE = 'https://solomon.yandex-team.ru/?project=Antiadblock&cluster=push&service=chevent_stats&l.sensor=unblock&l.device={}&l.scale=hour&l.service_id={}&graph=auto&b=1w&e='


logger = create_logger('money_monitoring_logger')

EVENTS = {
    'no_data': {
        'service': 'aab_report_no_data_{}_{}',
        'description': 'Для сервиса {} (device: {}) нет данных последние {} часов.\n',
        'ok_message': 'Data arrives normally',
    },
    'money_drop': {
        'service': 'aab_report_money_drop_{}_{}',
        'description': 'Падение денег на {}% на {} (device: {})\n',
        'ok_message': 'No sharp falls',
    },
    'negative_trend': {
        'service': 'aab_report_negative_trend_{}_{}',
        'description': 'Негативный тренд на {}% на {} (device: {})\n',
        'ok_message': 'No negative trends',
    },
}


def create_ticket(service_id, device, ticket_type):
    tvm_settings = TvmApiClientSettings(
        self_tvm_id=int(os.getenv('TVM_ID')),
        self_secret=os.getenv('TVM_SECRET'),
        enable_service_ticket_checking=False,
        dsts=dict(configs_api=int(os.getenv('CONFIGSAPI_TVM_ID'))))
    return post_tvm_request('https://api.aabadmin.yandex.ru/service/{}/create_ticket'.format(service_id), TvmClient(tvm_settings),
                            {'type': ticket_type,
                             'device': device})


def send_statuses_to_juggler(ids_state):
    total_no_data = float(FAILED_CHECKS['no_data']) / len(ids_state) > 0.5
    total_money_drop = float(FAILED_CHECKS['money_drop']) / len(ids_state) > 0.5
    send_event(
        host=JUGGLER_MONEY_HOST, service='aab_total_no_data',
        status='CRIT' if total_no_data else 'OK',
        description='Total no data\nПожаловаться на алерт: {}'.format(CLAIM_LINK_TEMPLATE.format(JUGGLER_MONEY_HOST,
                                                                                                 'aab_total_no_data',
                                                                                                 get_datetime_for_incident_start()))
    )
    send_event(
        host=JUGGLER_MONEY_HOST, service='aab_total_money_drop',
        status='CRIT' if total_money_drop else 'OK',
        description='Total money drop\nПожаловаться на алерт: {}'.format(CLAIM_LINK_TEMPLATE.format(JUGGLER_MONEY_HOST,
                                                                                                    'aab_total_money_drop',
                                                                                                    get_datetime_for_incident_start()))
    )
    if total_no_data or total_money_drop:
        return
    for service_id, device in ids_state.keys():
        for check_state in ids_state[(service_id, device)]:
            service = EVENTS[check_state['check']]['service'].format(service_id, device)
            ticket_id = 'NOTICKET'
            if check_state['status'] == 'CRIT':
                if check_state['check'] != 'no_data':
                    response = create_ticket(service_id, device, check_state['check'])
                    if response.status_code == TICKET_ALREADY_EXISTS:
                        continue
                    elif response.status_code in (TICKET_CREATED, TICKET_CHANGED):
                        ticket_id = response.json().get('ticket_id', ticket_id)
                    report_url = SOLOMON_GRAPH_TEMPLATE.format(device, service_id)
                    description = '.\n{}\nТикет: https://st.yandex-team.ru/{}\nГрафик: {}'.format(
                        EVENTS[check_state['check']]['description'].format(check_state['info'], service_id, device),
                        ticket_id, report_url)
                    description += '\nПожаловаться на алерт: {}'.format(CLAIM_LINK_TEMPLATE.format(JUGGLER_MONEY_HOST,
                                                                                                   service,
                                                                                                   get_datetime_for_incident_start()))
                else:
                    description = EVENTS[check_state['check']]['description'].format(service_id, device, check_state['info'])
            else:
                description = EVENTS[check_state['check']]['ok_message']
            send_event(host=JUGGLER_MONEY_HOST, service=service, status=check_state['status'], description=description)


if __name__ == "__main__":
    stat = StatReports()
    logger.info('check if Stat report has data')
    data_delay = stat.get_money_data_delay()
    if data_delay >= ACCEPTABLE_DATA_DELAY * 60:
        send_event(host=JUGGLER_MONEY_HOST, service='aab_report_update', status='CRIT',
                   description=f'.\nОтчет AntiAdblock/partners_money_united не обновлялся более чем {int(data_delay)} мин.'
                   )
        exit(0)

    send_event(host=JUGGLER_MONEY_HOST, service='aab_report_update', status='OK')

    logger.info('downloading data from Stat')

    states = OrderedDict()
    money_data = stat.get_money_data()
    stat_service_ids = list(money_data.reset_index().service_id.unique())
    actual_service_monitoring_settings = get_monitoring_settings(int(os.getenv('TVM_ID')), os.getenv('TVM_SECRET'),
                                                                 int(os.getenv('CONFIGSAPI_TVM_ID')),
                                                                 os.getenv('CONFIGS_API_HOST'))
    service_ids = list(set(actual_service_monitoring_settings.keys()).intersection(set(stat_service_ids)))
    for service_id in service_ids:
        for device in actual_service_monitoring_settings[service_id]:
            partner_data = money_data.loc[service_id].sort_index()[:-1].query('device == "{}"'.format(device))
            logger.info(f'Checking {service_id}, device: {device} data range ({partner_data.index.min()}, {partner_data.index.max()}')
            states[(service_id, device)] = []
            # Список проверок с дополнительными аргументами
            all_checks = ((check_no_data, dict()),
                          (check_money_drop, dict(valuable=service_id in VALUABLE_IDS)),
                          (check_negative_trend, dict()))
            for check, kwargs in all_checks:
                check_result = None
                kwargs['partner_data'] = partner_data
                kwargs['service_id'] = service_id
                kwargs['device'] = device
                kwargs['stat_client'] = stat
                try:
                    check_result = check(**kwargs)
                except Exception as e:
                    logger.info('Error: {} {} {}'.format(check.__name__, service_id, str(e)), exc_info=True)
                if not check_result:
                    continue
                FAILED_CHECKS[check_result['check']] += int(check_result['status'] == 'CRIT')
                logger.info('check_result: ' + str(check_result))
                states[(service_id, device)].append(check_result)
    send_statuses_to_juggler(states)
