import requests

"""
    https://wiki.yandex-team.ru/solomon/api/data/
"""


def api_url(cluster, service, interval, tags):
    return "http://solomon.yandex.net/data-api/get?project=nbs&who=vasya&cluster=%s&service=%s&b=%s&%s" % (
        cluster, service, interval, "&".join("l.%s=%s" % tag for tag in tags))


def fetch(url):
    r = requests.get(url)
    r.raise_for_status()
    return r.json()["sensors"]
