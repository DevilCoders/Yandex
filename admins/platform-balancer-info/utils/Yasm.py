import requests


def memoize(f):
    memo = {}

    def helper(*args):
        key = '.'.join(args)
        if key not in memo:
            memo[key] = f(*args)
        return memo[key]

    return helper


@memoize
def get_monitoring(key):
    endpoint = f"https://yasm.yandex-team.ru/srvambry/get?key={key}"

    alerts = []
    try:
        res = requests.get(endpoint)
        res.raise_for_status()
        if res.json()['status'] != "ok":
            print(res.json())
            return
        charts = res.json()['result']['charts']
        for c in charts:
            if "type" in c and c["type"] == "alert":
                alerts.append(c)
        return alerts
    except requests.HTTPError:
        if res.status_code == 404:
            return alerts
    except:
        raise


@memoize
def list_alerts(prefix):
    endpoint = f"https://yasm.yandex-team.ru/srvambry/alerts/list"
    data = {
        # 'itype': "qloud",
        'name_prefix': prefix
    }
    ret = requests.get(endpoint, params=data)
    ret.raise_for_status()
    if ret.json()['status'] != 'ok':
        print(ret.json())
        return
    return ret.json()['response']['result']


if __name__ == '__main__':
    alerts = list_alerts('kinopoisk.ab-testing.testing.backend')
    for alert in alerts:
        print(alert['name'])
