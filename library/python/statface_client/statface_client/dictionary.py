from __future__ import division, absolute_import, print_function, unicode_literals


class StatfaceDictionary(object):
    ''' https://wiki.yandex-team.ru/statbox/Statface/externaldictionaries '''

    DICT_API = '/_api/dictionary/'

    def __init__(self, statface_client, name):
        self.client = statface_client
        self.name = name

    def download(self, language=None, out_format=None, metadata=None):
        params = {
            'name': self.name
        }
        if language is not None:
            params['language'] = language
        if out_format is not None:
            params['format'] = out_format
        if metadata is not None:
            params['metadata'] = metadata
        resp = self.client._request('get', self.DICT_API, params=params)
        return resp.content.decode('utf-8')

    def upload(self,  # pylint: disable=too-many-arguments
               data,
               language=None,
               in_format=None,
               out_format=None,
               description=None,
               load_to_mr=None,
               i18n=None,
               allow_all_edit=None,
               editors=None,
               metadata=0):
        send_data = {
            'name': self.name,
            'dictionary': data
        }
        if language is not None:
            send_data['language'] = language
        if in_format is not None:
            send_data['input_format'] = in_format
        if out_format is not None:
            send_data['format'] = out_format
        if description is not None:
            send_data['description'] = description
        if load_to_mr is not None:
            send_data['load_to_mr'] = load_to_mr
        if i18n is not None:
            send_data['i18n'] = i18n
        if allow_all_edit is not None:
            send_data['allow_all_edit'] = allow_all_edit
        if editors:
            send_data['editors'] = editors
        if metadata:
            send_data['metadata'] = metadata
        return self.client._request(
            'post',
            self.DICT_API,
            json=send_data)
