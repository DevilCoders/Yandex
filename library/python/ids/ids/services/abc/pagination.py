from six.moves.urllib import parse

from ids.lib.linked_pagination import LinkedPageFetcher, LinkedPage


class ABCLinkedPage(LinkedPage):

    def set_page_attributes(self, data):
        self.results = data.pop('results')
        self.previous_page_url = data.pop('previous')
        self.next_page_url = data.pop('next')
        super(LinkedPage, self).set_page_attributes(data)


class LinkedPageAbcFetcher(LinkedPageFetcher):

    page_cls = ABCLinkedPage

    def get_request_params_for_page_after_cursor(self, next_page_url):
        cursor = parse.parse_qs(next_page_url.query).get('cursor')
        request_params = dict(self.request_params)
        query_params = request_params.get('params', {})
        query_params['cursor'] = cursor[0]
        return request_params

    def get_request_params_for_page_after(self, current):
        url = parse.urlparse(current.next_page_url)
        if url.path.startswith('/api/v4'):
            return self.get_request_params_for_page_after_cursor(url)
        else:
            return super(LinkedPageAbcFetcher, self).get_request_params_for_page_after(current)
