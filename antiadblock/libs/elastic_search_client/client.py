import requests
from retry import retry
from copy import deepcopy
from urllib3.exceptions import NewConnectionError

from antiadblock.libs.deploy_unit_resolver.lib import get_fqdns


class SearchClient(object):

    template_json = {
        "size": 10000,
        "version": True,
        "sort": [{"@timestamp": {"order": "desc", "unmapped_type": "boolean"}}],
        "docvalue_fields": [{"field": "@timestamp", "format": "date_time"}],
        "query": {
            "bool": {
                "must": [],
                "must_not": [],
                "should": [],
                "filter": [],
            }
        }
    }

    def __init__(self, name_dc=None, use_search_node=False, batch_size=10):
        self.batch_size = batch_size
        self._headers = {'X-AAB-ELASTICSEARCH': '1'}
        self.host = "https://ubergrep.yandex-team.ru"
        if use_search_node:
            self._headers = {}
            self.host = self.__init_search_node(name_dc)

    def __init_search_node(self, name_dc):
        nodes = [name_dc] if name_dc else ['vla', 'sas', 'man']
        return "http://{}:8890".format(get_fqdns('antiadb-elasticsearch', ['SearchNodes'], nodes)[0])

    @retry(exceptions=(requests.ConnectionError, NewConnectionError), tries=3, delay=2, backoff=2)
    def _get(self, request_json):
        response = requests.post(self.host + "/_search", json=request_json, headers=self._headers)
        response.raise_for_status()
        results = response.json().get('hits', {}).get('hits', [])
        return filter(None, [item.get("_source") for item in results])

    def get(self,
            range_dt,
            match_phrases=None,
            match_fields=None,
            ):
        request_json = deepcopy(self.template_json)
        if match_phrases:
            for item in match_phrases:
                request_json["query"]["bool"]["filter"].append(dict(match_phrase=item))
        request_json["query"]["bool"]["filter"].append({
            "range": {
                "@timestamp": {
                    "gte": range_dt["from"],
                    "lte": range_dt["to"],
                    "format": "strict_date_optional_time",
                }
            }
        })
        results = []
        if match_fields:
            for batch in range(0, len(match_fields), self.batch_size):
                filter_json = dict(bool=dict(should=[], minimum_should_match=1))
                for item in match_fields[batch:batch + self.batch_size]:
                    filter_json["bool"]["should"].append({
                        "bool": dict(should=[dict(match=item)], minimum_should_match=1)
                    })
                batch_request_json = deepcopy(request_json)
                batch_request_json["query"]["bool"]["filter"].append(filter_json)
                results.extend(self._get(batch_request_json))

        return results
