#!/usr/bin/env python3.6
"""
Selective env variable pass for infra tests
"""

import os
import time

import requests
from dateutil import parser as dt_parser


KNOWN_TAGS = [
    '[all]',
    '[clickhouse]',
    '[dataproc]',
    '[deploy_v2]',
    '[elasticsearch]',
    '[opensearch]',
    '[greenplum]',
    '[kafka]',
    '[mongodb]',
    '[mysql]',
    '[noci]',
    '[postgresql]',
    '[redis]',
    '[sqlserver]',
]


def get_timestamp(patch):
    """
    Get unix time of change from patch dict
    """
    parsed = dt_parser.parse(patch['created_time'])
    return int(parsed.timestamp())


def get_review_info(review_id, token):
    """
    Get latest available change for review_id
    """
    if review_id == 'trunk':
        return '', int(time.time())

    res = requests.get(
        f'https://a.yandex-team.ru/api/v1/pullrequest/{review_id}',
        headers={
            'Accept': 'application/json',
            'Authorization': f'OAuth {token}',
        })
    res.raise_for_status()
    data = res.json()
    force_tag = os.environ.get('FORCE_TAG')
    if force_tag:
        data['summary'] += ' [{tag}]'.format(tag=force_tag)

    return data['summary'], max([get_timestamp(x) for x in data['patches']])


def _main():
    review_id = os.environ.get('ARCANUMID', 'trunk')
    token = os.environ.get('ARCANUMAUTH')
    summary, max_change = get_review_info(review_id, token)
    data = [f'MAX_SEEN_CHANGE={max_change}\n', f'SUMMARY={summary}\n']
    if review_id != 'trunk':
        data.append(f'ARCANUMID={review_id}\n')
        has_tag = False
        for tag in KNOWN_TAGS:
            if tag in summary:
                has_tag = True
                break
        check_tags = os.environ.get('TAGS_ARE_MANDATORY', 'yes').lower() in ['yes', 'true', '1']
        if check_tags and not has_tag:
            raise RuntimeError(f'Unable to find tag in summary. Known tags: {", ".join(KNOWN_TAGS)}')
    else:
        data.append('ARCANUMID=trunk\n')
    with open('infra.properties', mode='w', encoding='utf-8') as out:
        out.writelines(data)


if __name__ == '__main__':
    _main()
