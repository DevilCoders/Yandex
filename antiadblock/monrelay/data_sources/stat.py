from copy import deepcopy
from datetime import datetime, timedelta

from . import DataSource

import statface_client


class DSStat(DataSource):

    def __init__(self, service_ids, config, logger, oauth_token):
        super(DSStat, self).__init__(service_ids, config, logger)
        self.__client = statface_client.StatfaceClient(host=statface_client.STATFACE_PRODUCTION, oauth_token=oauth_token)
        self.report = self.__client.get_report(self.args['report'])

    def calculate_status(self, row):
        if 'detect-checker-results' in self.args['report']:
            if (row['adblock'] == 'Without adblock' and row['cookiewait_detect_result'] == 'NOT_BLOCKED') or row['cookiewait_detect_result'] == 'BLOCKED':
                return 'green'
            else:
                return 'red'

    def get_check_result(self):
        result = list()
        stat_data = self.report.download_data(scale='i')
        for row in stat_data:
            service_id = row['service_id']
            check_id = '_'.join(['detect', row['adblock'].replace(' ', '_').lower(), row['browser'].lower()])
            if check_id == self.config['check_id'] and service_id in self.service_ids:
                subresult = deepcopy(self.config)
                valid_till = (datetime.strptime(row['fielddate'], "%Y-%m-%d %H:%M:%S") + timedelta(seconds=subresult.pop('ttl'))).strftime(self.datetime_format)
                subresult.update({'service_id': service_id,
                                  'state': self.calculate_status(row),
                                  'value': 'console_result: {}\ncookiewait_result:{}'.format(row['console_detect_result'], row['cookiewait_detect_result']),
                                  'last_update': row['fielddate'],
                                  'valid_till': valid_till,
                                  'external_url': self.report.report_url})
                result.append(subresult)
        return result
