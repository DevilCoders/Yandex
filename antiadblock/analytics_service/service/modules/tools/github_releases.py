# encoding=utf8
from datetime import datetime

import requests


def get_info_from_github(releases_url, use_prerelease=False):
    response = requests.get(releases_url).json()
    if releases_url.endswith('releases'):
        for elem in response:
            if elem['draft'] is False and elem['prerelease'] in [use_prerelease, False]:
                return dict(version=elem['tag_name'],
                            description=elem['body'],
                            url=elem['html_url'],
                            check_date=datetime.now().isoformat())

    if releases_url.endswith('tags'):
        commit_url = response[0]["commit"]["url"]
        tag = response[0]["name"]

        response = requests.get(commit_url).json()
        return dict(version=tag,
                    description=response["commit"]["message"],
                    url=response["html_url"],
                    check_date=datetime.now().isoformat())
