#!/usr/bin/env python3

import argparse
import codecs
import json
import logging

import requests


def upload_cron(cron_id, cron, token):
    assert cron_id
    assert cron
    assert token
    url = "http://metrics.yandex-team.ru/services/api/cron/serp/edit/?id={}".format(cron_id)
    headers = {
        "Authorization": "OAuth {}".format(token),
        "Content-Type": "application/json;charset=UTF-8",
    }
    logging.info("put %s", url)
    response = requests.put(
        url=url,
        headers=headers,
        data=json.dumps(cron),
        verify=False,
    )
    logging.info("response: %s", response)
    if not (200 <= response.status_code <= 299):
        raise Exception("Error {}:\n{}".format(response.status_code, response.text))


def upload_crons(cron_ids, token):
    for cron_id in cron_ids:
        file_path = "cron_{}.json".format(cron_id)
        with codecs.open(file_path, "r", "utf-8") as f:
            cron = json.load(f)
        upload_cron(cron_id, cron, token)


def main():
    parser = argparse.ArgumentParser(description="Upload crons to metrics.")
    parser.add_argument(
        "--token",
        type=str,
        required=True,
        help="OAuth token",
    )
    parser.add_argument(
        "--cron-ids",
        type=str,
        nargs="+",
        help="cron ids",
    )
    args = parser.parse_args()

    logging.basicConfig(format='[%(levelname)s] %(asctime)s - %(message)s', level=logging.INFO)
    upload_crons(args.cron_ids, token=args.token)


if __name__ == "__main__":
    main()
