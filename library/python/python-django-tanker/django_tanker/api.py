# coding: utf-8

import logging
import json
from lxml import etree
try:
    from urlparse import urljoin
except ImportError:
    from urllib.parse import urljoin

import requests

from django_tanker import __version__

log = logging.getLogger(__name__)


URLS = {
    'testing': 'https://tanker-api.test.yandex-team.ru/',
    'stable':  'https://tanker-api.yandex-team.ru/',
}
USER_AGENT = 'python-django-tanker/{0}'.format(__version__)


class APIError(Exception):
    pass


class Tanker(object):
    def __init__(self, project_id, base_url, token,
                 dry_run, include_unapproved):
        self.project_id = project_id
        self.base_url = base_url
        self.token = token
        self.dry_run = dry_run
        self.include_unapproved = include_unapproved

    def list(self):
        (resp, content) = self._request(
            '/v2/keysets/', 'GET',
            params={'project': self.project_id}
        )

        if resp.status_code == 200:
            try:
                data = json.loads(content)
            except ValueError as exc:
                log.exception('Could not parse content as JSON: %s', content)
                raise APIError('JSON parsing error: %s', exc)
            return dict((
                (keyset['name'], keyset['name'])
                for keyset in data['keysets']
            ))
        else:
            raise APIError('http error: %s' % resp.status_code)

    def create(self, keyset, language, path, key_not_language,
               file_format=None, branch=None):
        return self.upload(
            keyset, language,
            path, 'create',
            key_not_language,
            file_format,
            branch
        )

    def upload(self, keyset, language, file_info, mode, key_not_language,
               file_format=None, branch=None):
        params = {
            'project-id': self.project_id,
            'keyset-id': keyset,
            'language': language,
        }
        if branch is not None:
            params['branch-id'] = branch
        if key_not_language:
            params['original-not-id'] = 1
        if file_format:
            params['format'] = file_format

        if not self.dry_run:
            if isinstance(file_info, dict):
                file_description = (
                    file_info.get('filename', 'file'),
                    file_info['data'],
                    file_info.get('content_type')
                )
            else:
                path = file_info
                file_description = open(path, 'rb')
            (resp, content) = self._request(
                'keysets/%s/' % mode,
                'POST',
                data=params,
                files={'file': file_description}
            )
            self._check_error(resp)
        return True

    def download(self, keyset, language, key_not_language, status=None,
                 file_format='po', ref='master'):
        """
        @type ref: basestring
        @param ref: имя ветви, или идентификатор ревизии проекта.
        """
        params = {
            'project-id': self.project_id,
            'branch-id': ref,
            'keyset-id': keyset,
            'language': language,
        }
        if key_not_language:
            params['original-not-id'] = 1
        if status:
            params['status'] = status
        if self.include_unapproved:
            params['status'] = 'unapproved'

        (resp, content) = self._request(
            'keysets/%s/' % file_format,
            'GET',
            params=params
        )
        self._check_error(resp)
        return content

    def merge_branch(self, from_branch, destination):
        path = '/branches/merge/?'
        params = {
            'project-id': self.project_id,
            'source': from_branch,
            'dest': destination
        }
        (resp, content) = self._request(
            path,
            'POST',
            params=params
        )
        if resp.status_code > 299:
            raise APIError(
                'Could not merge branch {0} to {1}, '
                'unexpected API response: {2}'.format(
                    from_branch, destination, content
                )
            )

    def delete_branch(self, branch_name):
        path_to_branch = '/admin/project/{project}/branch/{branch}'.format(
            project=self.project_id,
            branch=branch_name
        )
        (resp, content) = self._request(
            path_to_branch,
            'DELETE'
        )
        if resp.status_code > 299:
            raise APIError(
                'Could not delete branch, '
                'unexpected API response: {0}'.format(content)
            )

    def branch_description(self, branch_name):
        path = '/admin/project/{project}/branch/{branch}/'.format(
            project=self.project_id,
            branch=branch_name,
        )
        (resp, content) = self._request(
            path,
            'GET',
        )
        if resp.status_code < 299:
            data = json.loads(content)['data']
            return data
        return None

    def create_branch(self, branch_name, from_branch):
        """
        Создать ветку.
        """
        path = '/admin/project/{project}/branch/'.format(
            project=self.project_id,
        )
        (resp, content) = self._request(
            path,
            'POST',
            data=json.dumps({'name': branch_name, 'ref': from_branch})
        )
        print(content)
        if resp.status_code > 299:
            raise APIError(
                'Could not create branch, '
                'unexpected API response: {0}'.format(content)
            )

    def _request(self, path, method, params=None,
                 data=None, files=None, headers=None):
        url = urljoin(self.base_url, path)
        if headers is None:
            headers = dict()
        headers['User-Agent'] = USER_AGENT
        if self.token:
            headers['Authorization'] = self.token
        log.debug('Issuing: %s %s %s %s', method, url, data or {}, files or {})
        response = requests.request(
            method,
            url,
            params=params,
            headers=headers,
            files=files,
            data=data,
            verify='/etc/ssl/certs/ca-certificates.crt'
        )
        return (response, response.text)

    def _check_error(self, resp):
        content = bytes(bytearray(resp.text, encoding='utf-8'))
        # this is a workaround for elementtree stupidity
        # see: https://nda.ya.ru/3SvUho
        if resp.status_code in (200, 400):
            if resp.headers.get('content-type') == 'application/xml':
                xpath = etree.XPathEvaluator(
                    etree.fromstring(content)
                )
                status = xpath('//result/@status')[0]
                if status != 'ok':
                    raise APIError(xpath('//result/error/text()')[0])
        else:
            raise APIError('http error: %s' % resp.status_code)


if __name__ == '__main__':
    import code
    t = Tanker(
        'tanker_dev_test',
        URLS['testing'],
        token=None,
        dry_run=False,
        include_unapproved=True
    )
    code.interact(local=locals())
