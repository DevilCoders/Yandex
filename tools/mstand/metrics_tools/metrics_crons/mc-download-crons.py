#!/usr/bin/env python3

import argparse
import codecs
import json
import logging

import requests


def get_cron(cron_id, token):
    assert cron_id
    assert token
    url = "http://metrics.yandex-team.ru/services/api/cron/serp/info/?id={}".format(cron_id)
    headers = {"Authorization": "OAuth {}".format(token)}
    logging.info("get %s", url)
    response = requests.get(
        url=url,
        headers=headers,
        verify=False,
    )
    logging.info("response: %s", response)
    if not (200 <= response.status_code <= 299):
        raise Exception("Error {}:\n{}".format(response.status_code, response.text))
    return response.json()


def download_crons(cron_ids, token):
    for cron_id in cron_ids:
        file_path = "cron_{}.json".format(cron_id)
        cron = get_cron(cron_id, token)
        with codecs.open(file_path, "w", "utf-8") as f:
            json.dump(cron, f, indent=1, sort_keys=True)


def main():
    parser = argparse.ArgumentParser(description="Download crons from metrics.")
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
    download_crons(args.cron_ids, token=args.token)


if __name__ == "__main__":
    main()
