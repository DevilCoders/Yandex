# -*- coding: utf8 -*-
from urlparse import urljoin
from datetime import datetime, timedelta
from copy import deepcopy
from multiprocessing.dummy import Pool as ThreadPool

import requests

from . import DataSource

SOLOMON_URL_TEMPLATE = 'https://solomon.yandex-team.ru/{}/projects/Antiadblock/alerts/'
SOLOMON_API_URL = SOLOMON_URL_TEMPLATE.format('api/v2')
SOLOMON_ADMIN_URL = SOLOMON_URL_TEMPLATE.format('admin')
SOLOMON_STATE_POSTFIX = "/state/evaluation"
DEFAULT_REQUEST_TIMEOUT = 60.0
SOLOMON_STATUS_MAP = {'OK': 'green',
                      'ALARM': 'red'}


class FailedRequestException(Exception):
    pass


class DSSolomon(DataSource):
    """
    Provides object to get data from solomon by url
    """

    def __init__(self, service_ids, config, logger, oauth_token):
        super(DSSolomon, self).__init__(service_ids, config, logger)
        self.alert_id = self.args['alert_id']
        self.solomon_request_headers = {'Content-Type': 'application/json',
                                        'Accept': 'application/json',
                                        'Authorization': 'OAuth ' + oauth_token}

    def __subalert_filter(self, subalert):
        """
            Фильтр для сабалертов
            1. С существующим service_id
            2. С заданными в параметрах лейблами
            3. С данными
        """
        valid_service_id = subalert['labels']['service_id'] in self.service_ids
        check_labels = self.args.get('alert_labels') is None
        if not check_labels:
            check_labels = all([subalert['labels'].get(k) == v for k, v in self.args['alert_labels'].iteritems()])
        return valid_service_id and check_labels and subalert['evaluationStatusCode'] != 'NO_DATA'

    def get_check_result(self):
        try:
            subalerts = self._get_all_subalerts_for_alert(urljoin(SOLOMON_API_URL, '{}/subAlerts'.format(self.alert_id)))
            self.logger.info('')
            if not subalerts:
                self.logger.warning('There are no sub alerts in alert {}'.format(self.alert_id))
                return []
            result = self._get_all_subalerts_full_details(subalerts)
            return result
        except FailedRequestException:
            return []

    def _request_solomon_url(self, url):
        response = requests.get(url, headers=self.solomon_request_headers, timeout=DEFAULT_REQUEST_TIMEOUT)

        if response.status_code != 200:
            self.logger.error("Request to Solomon failed, error code: {}, error url: {}".format(response.status_code, url))
            raise FailedRequestException
        return response

    def _parse_alert_datetime(self, date):
        try:
            result = datetime.strptime(date[:19], self.datetime_format[:-1])
        except ValueError:
            result = None
        return result

    def _get_all_subalerts_for_alert(self, subalerts_url):
        subalerts = list()
        i = 0
        while True:
            response = self._request_solomon_url(urljoin(subalerts_url, '?pageSize=50&pageToken=' + str(i)))

            if len(response.json()['items']) != 0:
                subalerts += response.json()['items']
                i += 50
                continue
            else:
                break
        return subalerts

    def _get_all_subalerts_full_details(self, subalerts):
        result = list()

        # получение деталей проверки (поле dashboard_details в аннотациях)
        def get_subalert_detail(service_id, url):
            try:
                details = self._request_solomon_url(url)
            except FailedRequestException:
                return service_id, 'failed to load'
            return service_id, details.json()['status'].get('annotations', {}).get('dashboard_details', '')

        # запросим все детали всех ответов параллельно чтоб вставить в результат ниже
        subalert_details_urls = [(subalert['labels']['service_id'],
                                  urljoin(urljoin(SOLOMON_API_URL, '{}/subAlerts'.format(self.alert_id)) + '/',
                                          subalert['id']) + SOLOMON_STATE_POSTFIX) for subalert in subalerts if
                                 self.__subalert_filter(subalert)]
        if not subalert_details_urls:
            return result

        pool = ThreadPool(len(subalert_details_urls))
        results = pool.map(lambda args: get_subalert_detail(*args), subalert_details_urls)
        pool.close()
        pool.join()

        for service_id, detail in results:
            subalert_result = filter(lambda x: x['labels']['service_id'] == service_id, subalerts)
            if len(subalert_result) == 0:
                self.logger.warning("No one subalert for service_id: {} in alert: {}".format(service_id, self.alert_id))
                continue
            else:
                subalert_result = subalert_result[0]

            subresult = deepcopy(self.config)

            last_update = self._parse_alert_datetime(subalert_result['latestEval'])
            if last_update is not None:
                valid_till = (last_update + timedelta(seconds=int(subresult.pop('ttl')))).strftime(self.datetime_format)
            else:
                self.logger.error("Can't parse datetime: {}, alert_id: {}, service_id: {}".format(subalert_result['latestEval'], self.alert_id, service_id))
                continue

            subresult.update({'service_id': service_id,
                              'state': SOLOMON_STATUS_MAP.get(subalert_result['evaluationStatusCode'], 'red'),
                              'value': detail,
                              'last_update': subalert_result['latestEval'],
                              'valid_till': valid_till,
                              'external_url': urljoin(urljoin(SOLOMON_ADMIN_URL, '{}/subAlerts/'.format(self.alert_id)),
                                                      subalert_result['id'])})
            result.append(subresult)
        return result
