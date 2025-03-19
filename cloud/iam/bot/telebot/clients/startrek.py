import json
import requests

from cloud.iam.bot.telebot.clients import OAuthClient


class Startrek(OAuthClient):
    def __init__(self, config, token):
        super().__init__(config, token)

        self.ui_url = config['ui_url']

    def worklog(self, login, from_date, to_date):
        params = {'perPage': 100, 'page': 1}
        data = {'createdBy': login, 'start': {'from': from_date.strftime('%Y-%m-%d') + 'T00:00:00.000',
                                              'to': to_date.strftime('%Y-%m-%d') + 'T00:00:00.000'}}
        result = []
        print(data)
        while True:
            response = self.session.post(self._config['api'] + 'worklog/_search',
                                         params=params,
                                         json=data,
                                         timeout=self._config.get('timeout'))
            if response.status_code != requests.codes.ok:
                response.raise_for_status()

            items = json.loads(response.text)
            for item in items:
                result.append(item)

            if len(items) == 0:
                break

            params['page'] = params['page'] + 1

        return result

    def find_worklog(self, issue_key, worklog_id):
        response = self.session.get(self._config['api'] + 'issues/' + issue_key + '/worklog/' + worklog_id,
                                    timeout=self._config.get('timeout'))
        if response.status_code in {requests.codes.not_found, requests.codes.forbidden}:
            return False
        if response.status_code != requests.codes.ok:
            response.raise_for_status()

        return json.loads(response.text)

    def delete_worklog(self, issue_key, worklog_id):
        response = self.session.delete(self._config['api'] + 'issues/' + issue_key + '/worklog/' + worklog_id,
                                       timeout=self._config.get('timeout'))
        if response.status_code in {requests.codes.not_found, requests.codes.forbidden}:
            return False
        elif response.status_code != requests.codes.ok:
            response.raise_for_status()

        return True
