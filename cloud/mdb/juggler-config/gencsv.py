#!/usr/bin/env python3
"""
Generate duty csv for cloud bot
"""
import argparse
import os
from datetime import datetime, timedelta

import yaml

IGNORE = ['asalimonov']

RESPS_FILES = {
    'yc_mdb_postgresql.csv':    'gendb_resps.yml',
    'yc_mdb_mysql.csv':         'gendb_resps.yml',
    'yc_mdb_clickhouse.csv':    'specdb_resps.yml',
    'yc_mdb_mongodb.csv':       'specdb_resps.yml',
    'yc_mdb_redis.csv':         'specdb_resps.yml',
    'yc_mdb_core.csv':          'core_resps.yml',
    'yc_dataproc.csv':          'dataproc_resps.yml',
}


def main():
    """
    Program entry point.
    """
    args = parse_args()

    for out_file, in_file in RESPS_FILES.items():
        resps = get_resps(in_file)
        gen_csv(out_file, resps, args.start, args.duration)


def parse_args():
    """
    Parse command-line arguments.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-d', '--duration', type=int, default=7,
        help='Duty duration, in days. Default: 7.')
    parser.add_argument(
        '-s', '--start',
        help='Start date of duty schedule, in the format YYYY-MM-DD. Default: Monday of the current week.')
    return parser.parse_args()


def get_resps(filename):
    """
    Get resps and filter out ignored ones
    """
    dirname = os.path.dirname(__file__)
    with open(os.path.join(dirname, 'configs', filename)) as resps:
        data = yaml.safe_load(resps)
    resps_list = data['resps']
    return [x for x in resps_list if x not in IGNORE]


def generate_rows(resps, start_date, duty_duration):
    """
    Get duty rows (date, primary duty, backup duty)
    """
    result = []

    total_resps = len(resps)

    if start_date:
        date = datetime.strptime(start_date, '%Y-%m-%d').date()
    else:
        today = datetime.now().date()
        date = today - timedelta(days=today.weekday())

    for i in range(2 * total_resps - 1):
        primary = resps[i % total_resps]
        backup = resps[(i+1) % total_resps]
        for offset in range(duty_duration):
            result.append((date + timedelta(days=offset), primary, backup))
        date += timedelta(days=duty_duration)

    return result


def gen_csv(filename, resps, start_date, duty_duration):
    """
    Generate single csv file
    """
    with open(filename, 'w') as out:
        out.write('Date,Primary,Backup\n')
        for row in generate_rows(resps, start_date, duty_duration):
            out.write('{date},staff:{primary},staff:{backup}\n'.format(
                date=row[0].strftime('%d/%m/%Y'), primary=row[1], backup=row[2]))


if __name__ == '__main__':
    main()
