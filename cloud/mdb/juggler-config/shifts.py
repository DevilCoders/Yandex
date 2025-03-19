"""
Example:
    JUGGLER_OAUTH_TOKEN="foo-bar" python3 ./shifts.py corescrumteam duty ./configs/core_resps.yml
"""
import os
from datetime import datetime, timedelta, timezone
from sys import argv
from typing import List
from urllib.parse import urlencode

import dateutil.parser
import requests
import yaml


def make_url(service_slug: str, schedule_slug: str, from_now: datetime) -> str:
    """

    request example: https://abc-back.yandex-team.ru/api/v4/duty/shifts/?fields=person.uid,person.login,replaces.person.uid,replaces.person.login,&date_from=2020-10-07&date_to=2020-10-20&service__slug=dataprocessing&schedule__slug=duty
    """
    date_from = from_now.strftime('%Y-%m-%d')
    date_to = (from_now + timedelta(days=30)).strftime('%Y-%m-%d')
    params = {
        'service__slug': service_slug,
        'schedule__slug': schedule_slug,
        'date_from': date_from,
        'date_to': date_to,
        'fields': ','.join((
            'person.uid',
            'person.login',
            'replaces.person.uid',
            'replaces.person.login'
        ))
    }
    result = 'https://abc-back.yandex-team.ru/api/v4/duty/shifts/?' + urlencode(params)
    return result


class Shift:
    login: str
    starts: datetime
    ends: datetime
    replaces: list

    def __init__(self, login: str, starts: datetime, ends: datetime, replaces: list):
        self.login = login
        self.starts = starts
        self.ends = ends
        self.replaces = replaces

    @staticmethod
    def from_api(raw_data: dict):
        return Shift(
            login=raw_data['person']['login'],
            starts=dateutil.parser.parse(raw_data['start_datetime']),
            ends=dateutil.parser.parse(raw_data['end_datetime']),
            replaces=list(map(Shift.from_api, raw_data.get('replaces', []))),
        )

    def dry_copy_with_new_borders(self, starts: datetime, ends: datetime):
        return Shift(
            login=self.login,
            starts=starts,
            ends=ends,
            replaces=[],
        )

    def __repr__(self):
        return '{}: {} ... {}'.format(
            self.login,
            self.starts,
            self.ends,
        )


def get_shifts_from_response(response_json):
    result = []
    for raw_shift in response_json['results']:
        if raw_shift['person'] is None:
            continue
        shift = Shift.from_api(raw_shift)
        result.append(shift)
    return result


def get_actual_shifts(shifts: List[Shift]):
    for shift in shifts:
        if shift.replaces:
            last_slot = None
            for replace_shift in shift.replaces:
                if last_slot is None:
                    last_slot = shift.dry_copy_with_new_borders(
                        starts=shift.starts, ends=shift.replaces[0].starts
                    )
                    if last_slot.starts != last_slot.ends:
                        yield last_slot
                if last_slot.ends < replace_shift.starts:
                    yield shift.dry_copy_with_new_borders(
                        starts=last_slot.ends, ends=replace_shift.starts
                    )
                last_slot = replace_shift
                yield replace_shift
            if shift.replaces[-1].ends < shift.ends:
                # right border of replacements
                yield shift.dry_copy_with_new_borders(
                    starts=shift.replaces[-1].ends, ends=shift.ends
                )
        else:
            yield shift


def generate_resps(service_slug, schedule_slug, from_now, token, output_file, append_always):
    response = requests.get(
        make_url(service_slug=service_slug, schedule_slug=schedule_slug, from_now=from_now),
        headers={'Authorization': 'OAuth {}'.format(token)}
    )
    response.raise_for_status()
    shifts = [s
              for s in get_actual_shifts(get_shifts_from_response(response.json()))
              if s.ends > from_now]
    resps = []
    for shift in shifts:
        if shift.login not in resps:
            resps.append(shift.login)
    for resp in append_always:
        if resp not in resps:
            resps.append(resp)
    data = dict(resps=resps)
    with open(output_file, 'w') as fp:
        yaml.dump(data, fp, indent=4)


def main():
    service_slug = argv[1]
    schedule_slug = argv[2]
    output_file = argv[3]
    token = os.environ['JUGGLER_OAUTH_TOKEN']

    generate_resps(
        service_slug=service_slug,
        schedule_slug=schedule_slug,
        from_now=datetime.now(timezone(timedelta(hours=3))),
        output_file=output_file,
        token=token,
        append_always=[],
    )


if __name__ == '__main__':
    main()
