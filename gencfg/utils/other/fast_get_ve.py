#!/skynet/python/bin/python
import requests
from subprocess import Popen, PIPE
import re
import warnings

url = 'https://sandbox.yandex-team.ru/api/v1.0/'


class Reqs(object):
    def __init__(self, dct=None):
        self.dct = dct or {}

    @classmethod
    def parse(cls, text):
        self = cls()
        for line in text.split('\n'):
            line = line.strip()
            if not line:
                continue
            t = re.match('[^ =<>]+', line)
            if t:
                self.dct[line[:t.end()]] = line[t.end():].strip()
            else:
                self.dct[line] = ''
        return self

    def __eq__(self, other):
        for key in self.dct:
            if key not in other.dct:
                return False
            newer = self.dct[key]
            older = other.dct[key]
            if newer != '' and newer != older:
                return False
        return True

    @staticmethod
    def read_reqs(path):
        with open(path) as f:
            res = f.read()
        return res


def _find_venv(searching_reqs):
    lst = requests.get(url + 'task?type=CREATE_VENV&limit=100&status=SUCCESS').json()['items']
    for task in lst:
        task = requests.get(task['url']).json()
        reqs = requests.get(task['context']['url']).json()['requirements_list']
        reqs = Reqs.parse(reqs)
        if searching_reqs == reqs:
            resources = requests.get(task['resources']['url']).json()['items']
            for res in resources:
                if res['type'] == 'OTHER_RESOURCE':
                    if res['state'] == 'READY':
                        return res['http']['proxy']
                    return None
    return None


def find_venv():
    searching = Reqs.parse(Reqs.read_reqs('requirements.txt'))
    res = _find_venv(searching)
    if res:
        Popen(['sky', 'get', res]).communicate()
        Popen(['tar', 'xvzf', res.split('/')[-1]], stdout=PIPE).communicate()
        Popen(['rm', res.split('/')[-1]]).communicate()
    else:
        Popen(['./get_ve.sh']).communicate()
    return


if __name__ == '__main__':
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        find_venv()
