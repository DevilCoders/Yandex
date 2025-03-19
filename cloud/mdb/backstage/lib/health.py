import metrika.pylib.http as http

from django.conf import settings


def chunks(elements, chunk_size):
    for i in range(0, len(elements), chunk_size):
        yield elements[i:i + chunk_size]


class HealthApi:
    def __init__(
        self,
        url=settings.CONFIG.health.api_url,
        ca_file=settings.CONFIG.health.get('ca_file'),
    ):
        self.url = url
        if ca_file:
            self.verify = ca_file
        else:
            self.verify = True

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
            verify=self.verify,
            timeout=(1, 1),
            retry_kwargs={'tries': 1},
            **kwargs
        )
        return response, url

    def hostshealth(self, hostnames):
        path = '/hostshealth'

        result = []
        for chunk in chunks(hostnames, 100):
            params = {'fqdns': ','.join(chunk)}
            response, url = self.request(path, params=params)
            result.extend(response.json()['hosts'])

        return result, url

    def clusterhealth(self, cid):
        path = '/clusterhealth'
        params = {'cid': cid}
        response, url = self.request(path, params=params)
        return response.json(), url

    def unhealthyaggregatedinfo(
        self,
        agg_type,
        c_type,
        env,
    ):
        path = '/unhealthyaggregatedinfo'
        params = {
            'agg_type': agg_type,
            'c_type': c_type,
            'env': env,
        }
        response, url = self.request(path, params=params)
        return response.json(), url
