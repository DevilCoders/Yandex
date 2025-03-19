import logging

import requests

from .YasmCheck import YasmCheck
from .YasmDashboard import YasmDashboard


def memoize(f):
    memo = {}

    def helper(*args):
        key = '.'.join(args)
        if key not in memo:
            memo[key] = f(*args)
        return memo[key]

    return helper


class Yasm:
    endpoint = "http://yasm.yandex-team.ru/srvambry/"

    def __init__(self, logger=None):
        self.log = logger if logger else logging.getLogger()

    def _make_req(self, method, path, **kwargs) -> requests.Response:
        try:
            req = requests.request(method, self.endpoint + path, **kwargs)
            req.raise_for_status()
        except requests.HTTPError as exc:
            if 399 < req.status_code < 500:
                return req
            else:
                self.log.critical(exc)
        except requests.exceptions.ConnectionError as exc:
            raise

        return req

    def create_alert(self, alert: YasmCheck):
        path = "alerts/create"
        self.log.info("Creating alert")
        ret = self._make_req("POST", path=path, data=alert.toJson())
        self.log.info(str(ret))

    def update_alert(self, alert: YasmCheck):
        path = "alerts/update"
        self.log.info("Updating alert")
        params = {'name': alert.name}
        ret = self._make_req("POST", path=path, data=alert.toJson(), params=params)
        if ret.json()['status'] == "ok":
            self.log.info("Updated successfully")
        else:
            self.log.error("Could not update")
            self.log.error(str(ret.json()))
        self.log.info(str(ret))

    def delete_alert(self, alert: YasmCheck):
        path = f"alerts/delete"
        params = {'name': alert.name}
        self.log.info(f"Deleting alert {alert.name}")
        ret = self._make_req("POST", path=path, params=params)
        if ret.json()['status'] == "ok":
            self.log.info("Deleted successfully ")
        else:
            self.log.error("Could not delete")
            self.log.error(str(ret.json()))

    def delete_panel(self, key, user):
        path = f"delete"
        params = {'name': key,
                  'user': f'{user}'}
        self.log.info(f"Deleting panel {user}.{key}")
        ret = self._make_req("POST", path=path, params=params)
        if ret.status_code < 300:
            self.log.info("Deleted successfully ")
        else:
            self.log.error("Could not delete")
            self.log.error(str(ret.json()))

    def create_panel(self, panel: YasmDashboard, user):
        path = 'upsert'
        data = f'{{"keys": {{"key": "{panel.key}", "user": "{user}"}}, "values": {panel.toJson()} }}'
        self.log.info(f"Creating panel {user}.{panel.key}")
        ret = self._make_req("POST", path=path,
                             data=f'{{"keys": {{"key": "{panel.key}", "user": "{user}"}}, "values": {panel.toJson()} }}')
        # if ret.json()['status'] == "ok":
        #     print(ret.json())
        #     self.log.info("Created successfully ")
        # else:
        #     self.log.error("Could not create")
        #     self.log.error(str(ret.json()))

    def get_panel(self, key):
        path = "get"
        params = {'key': key}
        self.log.info(f"Getting panel {key}")
        ret = self._make_req("GET", path=path, params=params)
        if ret.status_code < 400:
            self.log.info(f"Panel {key} exists")
            return ret.json()
        else:
            self.log.info(f"Panel {key} does not exists")
            print(ret.json())
            return None

    def get_alert(self, check: YasmCheck):
        path = f"alerts/get"
        params = {'name': check.name}
        self.log.info(f"Checking if alert {check.name} exists")
        ret = self._make_req("GET", path=path, params=params)
        if ret.status_code < 400:
            self.log.info(f"Check {check.name} exists")
            return ret.json()
        else:
            self.log.info(f"Check {check.name} does not exists")
            return None


if __name__ == '__main__':
    import sys

    logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)
    ys = Yasm()
    from pprint import pprint as pp

    pp(ys.get_alert('kinopoisk.ab-testing.cpu'))
    # pp(ys.delete_panel('kinopoisk-ab-testing'))
