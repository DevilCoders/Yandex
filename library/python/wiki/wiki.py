from __future__ import print_function
import requests
import json


class Grid(list):
    def __init__(self, wiki, name, fields, version):
        self.wiki = wiki
        self.name = name
        self.fields = fields
        self.version = version
        self.changed_rows = set()
        self.added_rows = set()
        self.removed_row_ids = set()

    def mark_changed(self, row_id):
        self.changed_rows.add(row_id)

    def commit(self):
        if not self.changed_rows and not self.added_rows and not self.removed_row_ids:
            return
        request = {'version': self.version, 'changes': []}

        for r in self:
            if id(r) in self.added_rows:
                request['changes'].append({'added_row': {
                    'after_id': r.after_id,
                    'data': r.get_data()}})
            if r.row_id in self.changed_rows:
                request['changes'].append({'edited_row': {
                    'id': r.row_id,
                    'data': r.get_data()}})

        for row_id in self.removed_row_ids:
            request['changes'].append({'removed_row': {
                'id': row_id}})

        result = self.wiki.rest_post(self.name, '.grid/change', data=json.dumps(request))

        self.added_rows = set()
        self.changed_rows = set()
        self.removed_row_ids = set()

        return result

    def new_row(self, cells):
        nr = Row(self, [{'row_id': -1, 'raw': cell} for cell in cells])
        nr.after_id = self[-1].row_id if self else -1
        self.append(nr)
        self.added_rows.add(id(nr))
        return nr

    def remove_row(self, id):
        self.removed_row_ids.add(id)


class Row(list):
    def __init__(self, page, row_data):
        self.page = page
        self.extend(c['raw'] for c in row_data)
        self.row_id = row_data[0]['row_id']

    def __setitem__(self, index, value):
        if list.__getitem__(self, index) != value:
            list.__setitem__(self, index, value)
            self.page.mark_changed(self.row_id)

    def get_data(self):
        return dict((self.page.fields[idx]['name'], value)
                    for idx, value in enumerate(self))

    def get_data_by_title(self):
        return dict((self.page.fields[idx]['title'], value)
                    for idx, value in enumerate(self))

    def remove_row(self):
        self.page.remove_row(self.row_id)


class File(object):
    def __init__(self, wiki, api_dict):
        self.wiki = wiki
        self.api_dict = api_dict

    def __repr__(self):
        return 'File({})'.format(self.api_dict['url'])

    def content(self):
        return self.wiki.file_content(self.api_dict['url'])


def get_json(r):
    try:
        r.raise_for_status()
    except requests.exceptions.HTTPError as err:
        print(err.response.text)
        raise
    return r.json()


class Wiki(object):
    def __init__(self, oauth_token):
        self.oauth_token = oauth_token

    url = 'https://wiki-api.yandex-team.ru/_api/frontend'

    def rest_get(self, page, api_method, **kwargs):
        return get_json(requests.get('/'.join((self.url, page, api_method)),
                                     headers={'Authorization': 'OAuth ' + self.oauth_token},
                                     **kwargs))

    def rest_post(self, page, api_method, data, **kwargs):
        return get_json(requests.post('/'.join((self.url, page, api_method)),
                                      headers={'Authorization': 'OAuth ' + self.oauth_token,
                                               'Content-Type': 'application/json'},
                                      data=data,
                                      **kwargs))

    def file_content(self, file_url):
        return requests.get(self.url + file_url,
                            headers={'Authorization': 'OAuth ' + self.oauth_token}
                            ).content

    def attachments(self, page_name):
        files = self.rest_get(page_name, '.files')['data']['data']
        return [File(self, f) for f in files]

    def grid_rows(self, page_name):
        grid = self.rest_get(page_name, '.grid')['data']
        page = Grid(self, page_name, grid['structure']['fields'], grid['version'])
        page.extend(Row(page, row_data) for row_data in grid['rows'])
        return page
