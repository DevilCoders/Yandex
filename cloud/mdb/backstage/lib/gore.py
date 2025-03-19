from django.conf import settings

import metrika.pylib.http as http


class GoreApi:
    def __init__(
        self,
        url=settings.CONFIG.gore.api_url,
    ):
        self.url = url

    def request(
        self,
        path,
        method='GET',
        **kwargs
    ):
        url = self.url + path
        response = http.request(
            url,
            method=method,
            timeout=(1, 1),
            retry_kwargs={'tries': 2},
            **kwargs
        )
        return response, url

    def services(self):
        path = '/services'
        response, url = self.request(path)
        return response.json()

    def duty(self):
        path = '/duty'
        response, url = self.request(path)
        return response.json()
