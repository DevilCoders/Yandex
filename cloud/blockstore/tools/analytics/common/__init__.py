import requests
import yt.wrapper as yt


def add_expiration_time(table, expiration_time):
    if expiration_time is not None:
        yt.set(yt.ypath_join(table, '@expiration_time'), expiration_time)


def http_get(url):
    r = requests.get(url)
    r.raise_for_status()
    return r.content
