# coding=utf-8
import os

import requests


def get_experiments_info(service_id, date_start):

    headers = {
        "Content-Type": "application/json",
        "Authorization": "OAuth {}".format(os.getenv('WIKI_TOKEN'))

    }
    # сраница на вики https://wiki.yandex-team.ru/antiadb/Эксперименты-партнеров/
    wiki_url = "https://wiki-api.yandex-team.ru/_api/frontend/antiadb/jeksperimenty-partnerov/.grid"

    response_json = requests.get(wiki_url, headers=headers).json()

    rows = response_json["data"]["rows"]
    # строка состоит из трек колонок (Партнер, Информация, service_id)

    for row in rows:
        if row[2]['raw'] == service_id:
            return row[1]["raw"].encode("utf-8")

    return "Информации нет"
