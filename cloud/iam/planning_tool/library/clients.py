# -*- coding: utf-8 -*-

import boto3
import json
import requests

from urllib.parse import urlparse, urlunparse

from urllib3.util.retry import Retry
from requests.adapters import HTTPAdapter

from cloud.iam.planning_tool.library.utils import styled_reprint, styled_print


class RestClient(object):
    def __init__(self, config):
        self._url = config['url']

        self.session = requests.Session()
        retries = Retry(total=10,
                        backoff_factor=0.1,
                        status_forcelist=[500, 502, 503, 504])
        self.session.mount(self._url, HTTPAdapter(max_retries=retries))

    @staticmethod
    def _spinning_cursor():
        while True:
            for cursor in '|/-\\':
                yield cursor


class StartrekClient(RestClient):
    def __init__(self, config, token):
        super(StartrekClient, self).__init__(config['api'])
        self._headers = {'Authorization': f'OAuth {token}'}
        self._spinner = self._spinning_cursor()

    def search(self, query, fields):
        print()
        params = {'query': query, 'scrollType': 'unsorted', 'perScroll': 1000, 'scrollTTLMillis': 13000,
                  'fields': fields}
        result: list[dict] = []

        def print_progress(n, done):
            styled_reprint(f'[{"+" if done else next(self._spinner)}] Searching for issues ({n} issues found)')

        while True:
            print_progress(len(result), False)
            response = self.session.get(f'{self._url}/issues', params=params, headers=self._headers)
            if response.status_code != requests.codes.ok:
                response.raise_for_status()

            items = json.loads(response.text)
            result.extend(items)

            if 'X-Scroll-Id' not in response.headers:
                break
            params = {'scrollId': response.headers['X-Scroll-Id'],
                      'scrollToken': response.headers['X-Scroll-Token']}

        print_progress(len(result), True)
        return result

    def find(self, issue_id, fields):
        response = self.session.get(f'{self._url}/issues/{issue_id}', headers=self._headers, params={'fields': fields})
        if response.status_code != requests.codes.ok:
            response.raise_for_status()

        return json.loads(response.text)

    def links(self, issue_id):
        response = self.session.get(f'{self._url}/issues/{issue_id}/links', headers=self._headers)
        if response.status_code != requests.codes.ok:
            response.raise_for_status()

        return json.loads(response.text)

    def remote_links(self, issue_id):
        response = self.session.get(f'{self._url}/issues/{issue_id}/remotelinks', headers=self._headers)
        if response.status_code != requests.codes.ok:
            response.raise_for_status()

        return json.loads(response.text)

    def worklog_by_person(self, person, from_date, to_date):
        print()
        params = {'perPage': 100, 'page': 1}
        data = {'createdBy': person, 'start': {'from': from_date.strftime('%Y-%m-%d') + 'T00:00:00.000',
                                               'to': to_date.strftime('%Y-%m-%d') + 'T00:00:00.000'}}
        result = []

        def print_progress(done):
            styled_reprint(f'[{("+" if done else next(self._spinner))}] Loading worklog of {person}')

        while True:
            print_progress(False)

            response = self.session.post(f'{self._url}/worklog/_search', params=params, json=data,
                                         headers=self._headers)
            if response.status_code != requests.codes.ok:
                response.raise_for_status()

            items = json.loads(response.text)
            for item in items:
                result.append(item)

            if len(items) == 0:
                break

            params['page'] = params['page'] + 1

        print_progress(True)

        return result

    def worklog(self, key):
        params = {'perPage': 100}  # + 'id': id предыдущей страницы
        result = []

        def print_progress(done):
            styled_reprint(f'[{"+" if done else next(self._spinner)}] Loading worklog of {key: <32}', )

        while True:
            print_progress(False)

            response = self.session.get(f'{self._url}/issues/{key}/worklog', params=params, headers=self._headers)
            if response.status_code != requests.codes.ok:
                response.raise_for_status()

            items = json.loads(response.text)
            for item in items:
                result.append(item)

            if len(items) == 0:
                break

            params['id'] = items[-1]['id']

        print_progress(True)

        return result

    def sprints(self, board_id, fields):
        styled_print(f'[-] Loading sprints of board {board_id}')
        response = self.session.get(f'{self._url}/boards/{board_id}/sprints', headers=self._headers,
                                    params={'fields': fields})
        if response.status_code != requests.codes.ok:
            response.raise_for_status()

        styled_reprint(f'[+] Loading sprints of board {board_id}')

        return json.loads(response.text)

    def sprint_issues(self, sprint_id):
        styled_print(f'[-] Loading issues of sprint {sprint_id}')
        response = self.session.get(f'{self._url}/sprints/{sprint_id}/issues', headers=self._headers)
        if response.status_code != requests.codes.ok:
            response.raise_for_status()

        styled_reprint(f'[+] Loading issues of sprint {sprint_id}')

        return json.loads(response.text)

    def get_comments(self, issue_id: str):
        response = self.session.get(f'{self._url}/issues/{issue_id}/comments?orderBy=updated', headers=self._headers)
        if response.status_code != requests.codes.ok:
            response.raise_for_status()

        return json.loads(response.text)

    def get_html_comment(self, issue_id: str, comment_id: int) -> str:
        response = self.session.get(f'{self._url}/issues/{issue_id}/comments/{comment_id}?expand=all',
                                    headers=self._headers)

        if response.status_code != requests.codes.ok:
            response.raise_for_status()

        return json.loads(response.text)['textHtml']


class GapClient(RestClient):
    def __init__(self, config, token):
        super(GapClient, self).__init__(config['api'])
        self._headers = {'Authorization': f'OAuth {token}'}

    # возвращает отсутствия в виде списка, отсутствие - это словарь
    def absences(self, from_date, to_date, logins):
        styled_print(f'[-] Loading absences {from_date}..{to_date}')
        date_from = from_date.strftime('%Y-%m-%d')
        date_to = to_date.strftime('%Y-%m-%d')
        response = self.session.get(f'{self._url}/gap-api/api/export_gaps',
                                    params={'date_from': date_from, 'date_to': date_to, 'l': logins},
                                    headers=self._headers)
        if response.status_code != requests.codes.ok:
            response.raise_for_status()

        absences = json.loads(response.text)['persons']

        styled_reprint(f'[+] Loading absences {from_date}..{to_date}')

        return absences


class StaffClient(RestClient):
    def __init__(self, config, token):
        super(StaffClient, self).__init__(config['api'])
        self._headers = {'Authorization': f'OAuth {token}'}

    def persons(self, logins):
        styled_print('[-] Loading persons')
        temp = map(lambda x: f'"{x}"', logins)
        response = self.session.get(f'{self._url}/v3/persons',
                                    params={'_query': f'login in [{",".join(temp)}]'},
                                    headers=self._headers)
        if response.status_code != requests.codes.ok:
            response.raise_for_status()

        persons = json.loads(response.text)['result']

        styled_reprint('[+] Loading persons')

        return persons


class WikiClient(RestClient):
    def __init__(self, config, token):
        super(WikiClient, self).__init__(config['api'])
        self._headers = {'Authorization': f'OAuth {token}'}

    def upload(self, path, title, body):
        styled_print('[-] Uploading wiki page')

        response = self.session.post(f'{self._url}/_api/frontend/{path}',
                                     headers=self._headers,
                                     data={'title': title, 'body': body})
        if response.status_code != requests.codes.ok:
            response.raise_for_status()

        styled_reprint('[+] Uploading wiki page')


class PasteClient(RestClient):
    def __init__(self, config, token):
        super(PasteClient, self).__init__(config['api'])
        self._headers = {'Authorization': f'OAuth {token}'}

    def paste(self, syntax, text):
        response = self.session.post(f'{self._url}/', headers=self._headers, data={'syntax': syntax, 'text': text},
                                     allow_redirects=False)
        if response.status_code != 302:
            raise RuntimeError(f'Could not upload to paste.yandex-team.ru: {response.text}')

        url = response.headers['Location']
        url_tuple = urlparse(url)
        if url_tuple.netloc:
            return url

        server_tuple = urlparse(self._url)
        url_tuple = (server_tuple[0], server_tuple[1],
                     url_tuple[2], url_tuple[3], url_tuple[4], url_tuple[5])

        return urlunparse(url_tuple)


class WebsiteClient:
    def __init__(self, config, secret_key_id, secret_key):
        session = boto3.session.Session(aws_access_key_id=secret_key_id, aws_secret_access_key=secret_key)
        self._s3 = session.client(service_name='s3', endpoint_url=config['endpoint'])
        self._bucket = config['bucket']

    def upload(self, key, body):
        self._s3.put_object(Bucket=self._bucket, Key=key, Body=body, ContentType='text/html; charset=utf-8')
