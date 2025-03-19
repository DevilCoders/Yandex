#!/usr/bin/env python
"""
Get duty dates for current user
"""

import os
from datetime import datetime, timedelta
from getpass import getuser

import yaml

CONFIGS = (
    'gendb_resps.yml',
    'specdb_resps.yml',
    'core_resps.yml',
    'dataproc_resps.yml',
)

def get_position():
    """
    Get current user position in duty list
    """
    dirname = os.path.dirname(__file__)
    for path in CONFIGS:
        try:
            with open(os.path.join(dirname, 'configs', path)) as resps:
                data = yaml.safe_load(resps)
            resps_list = data['resps']
            user = getuser()
            return resps_list.index(user)
        except ValueError:
            continue


def get_week_diff(position):
    """
    Get start day for first duty
    """
    today = datetime.now().date()
    week_start = today - timedelta(days=today.weekday())

    primary_duty_start = week_start + timedelta(days=7 * (position - 1))
    primary_duty_end = primary_duty_start + timedelta(days=7)
    backup_duty_start = primary_duty_end
    backup_duty_end = backup_duty_start + timedelta(days=7)
    return (primary_duty_start, primary_duty_end), (backup_duty_start, backup_duty_end)


def _main():
    backup, primary = get_week_diff(get_position())
    print(f'Backup:  Start {backup[0]} End {backup[1]}')
    print(f'Primary: Start {primary[0]} End {primary[1]}')


if __name__ == '__main__':
    _main()
