import requests


def getHosts(group, baseUrl='https://c.yandex-team.ru/api/groups2hosts/',
             fields=['fqdn', 'datacenter_name']):
    return requests.get(baseUrl + group + '?fields=' + ','.join(fields) +
                        '&format=json').json()
