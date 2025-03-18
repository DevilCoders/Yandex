#!/usr/bin/env python
# coding: utf8

from __future__ import division, absolute_import, print_function  # , unicode_literals

import argparse
import datetime as dt
import json

import pytz

import yt.wrapper as yt

from nile.api.v1 import clusters  # , Record
from nile.api.v1 import filters as nf
from nile.api.v1 import extractors as ne
from qb2.api.v1 import filters as qf
from qb2.api.v1.extractors.pool import datetime as qbdt

import statinfra.config

from nile.api.extensions.report_diff import CubeDiff

import statface_client
import statface_client.constants

from copy_report_cube import download_cube


MSK = pytz.timezone('Europe/Moscow')
DEFAULT_RESULT_PATH = '//statbox/statbox/tmp/report_diff'


# TODO: refactor into multiple functions
def create_diff(**params):  # pylint: disable=too-many-locals,too-many-statements
    OLD_REPORT_PATH = params['old_report_path']
    NEW_REPORT_PATH = params['new_report_path']
    OLD_REPORT_PROXY = params['old_report_proxy']
    NEW_REPORT_PROXY = params['new_report_proxy']

    YT_PATH = params['yt_path_prefix']
    OLD_CUBE_YT_PATH = params['old_cube_yt_path']
    NEW_CUBE_YT_PATH = params['new_cube_yt_path']

    YT_TOKEN = statinfra.config.get_config_from_env(
        'OAuth/tokens.tyaml'
    )['robot-statbox-copy']

    START_DATE = params['startdate']
    END_DATE = params['enddate']
    # SCALE = params['scale']  # Unused
    MEASURE = params['measure']

    old_report = statface_client.StatfaceClient(
        host=OLD_REPORT_PROXY
    ).get_report(
        path=OLD_REPORT_PATH
    )
    new_report = statface_client.StatfaceClient(
        host=NEW_REPORT_PROXY
    ).get_report(
        path=NEW_REPORT_PATH
    )

    old_dimensions = list(
        key.encode('utf-8') for key in
        old_report.config.dimensions.keys())
    old_measures = list(
        key.encode('utf-8') for key in
        old_report.config.measures.keys())
    new_dimensions = list(
        key.encode('utf-8') for key in
        new_report.config.dimensions.keys())
    new_measures = list(
        key.encode('utf-8') for key in
        new_report.config.measures.keys())

    common_measures = list(set(old_measures) & set(new_measures))

    old_columns_to_extract = (
        [key for key in old_dimensions if key != 'fielddate'] +
        common_measures)
    new_columns_to_extract = (
        [key for key in new_dimensions if key != 'fielddate'] +
        common_measures)

    columns_to_add_into_old = list(
        set(new_columns_to_extract) - set(old_columns_to_extract))
    columns_to_add_into_new = list(
        set(old_columns_to_extract) - set(new_columns_to_extract))

    if MEASURE is None:
        if len(common_measures) == 1:
            MEASURE = common_measures[0]
        else:
            raise create_parser().error(
                ('specify measure to compare.'
                 ' choose from {}').format(common_measures))

    OLD_VALUE = MEASURE + '_do'
    NEW_VALUE = MEASURE + '_dn'
    DIFF = "da_" + MEASURE
    UNSIGN_DIFF = "abs_" + DIFF
    RELATIVE_DIFF = "dp_" + MEASURE
    UNSIGN_RELATIVE_DIFF = "abs_" + RELATIVE_DIFF

    just_new_path = YT_PATH + '/just_new'
    just_old_path = YT_PATH + '/just_old'
    diff_path = YT_PATH + '/cube_diff'

    diff_path_srt_by_reldiff = '{}_sorted_by_relative_diff'.format(diff_path)
    diff_path_srt_by_ureldiff = '{}_sorted_by_unsigned_relative_diff'.format(diff_path)
    diff_path_srt_by_absdiff = '{}_sorted_by_unsigned_absolute_diff'.format(diff_path)

    # TODO: refactor more
    # pylint: disable=too-many-locals
    def calculate():

        def dt_to_msk_ts(fielddate):
            """ convert datetime to moscow timestamp """
            utc_date = dt.datetime.strptime(fielddate, '%Y-%m-%d')
            moscow_datetime = qbdt.extract_datetime_object_with_timezone(
                utc_date, pytz.utc, MSK)
            moscow_timestamp = qbdt.extract_timestamp_from_datetime_object(
                moscow_datetime)
            return moscow_timestamp

        def msk_ts_to_dt(moscow_timestamp):
            """ convert moscow timestamp to datetime """
            utc_datetime = qbdt.extract_datetime_object_from_timestamp(
                moscow_timestamp)
            moscow_datetime = qbdt.extract_datetime_object_with_timezone(
                utc_datetime, MSK, pytz.utc)
            return qbdt.extract_datetime_from_datetime_object(
                moscow_datetime, '%Y-%m-%d')

        def cut_a_piece_of_cube(streamed_cube, start_date, end_date,
                                columns_to_extract, columns_to_add):
            start_date_ts = dt_to_msk_ts(start_date)
            end_date_ts = dt_to_msk_ts(end_date)

            columns = {k: ne.const('_total_') for k in columns_to_add}
            columns['fielddate'] = ne.custom(msk_ts_to_dt, 'fielddate')
            return (
                streamed_cube
                .filter(
                    nf.custom(
                        lambda date: start_date_ts <= date <= end_date_ts,
                        'fielddate'))
                .project(*columns_to_extract, **columns))

        CLUSTER = clusters.yt.Hahn(
            token=YT_TOKEN,
            pool='statbox_remote_copy',
        ).env(
            parallel_operations_limit=10,
        )

        job = CLUSTER.job()

        new_cube = job.table(NEW_CUBE_YT_PATH)
        filtered_new_cube = cut_a_piece_of_cube(
            new_cube,
            START_DATE,
            END_DATE,
            new_columns_to_extract,
            columns_to_add_into_new,
        )
        old_cube = job.table(
            OLD_CUBE_YT_PATH,
        )
        filtered_old_cube = cut_a_piece_of_cube(
            old_cube,
            START_DATE,
            END_DATE,
            old_columns_to_extract,
            columns_to_add_into_old,
        )

        def encode_dict(flat_dictionary):
            return {k.encode('utf-8'): v.encode('utf-8')
                    for k, v in flat_dictionary.items()}

        measures = encode_dict(
            {k: v for k, v in old_report.config.measures.items() if
             k in common_measures}
        )
        all_dimensions = dict(new_report.config.dimensions)
        all_dimensions.update(old_report.config.dimensions)
        dimensions = encode_dict({k: v for k, v in all_dimensions.items()})

        cube_diff = CubeDiff(dimensions, measures, threshold=0)

        diff_stream = filtered_old_cube.call(
            cube_diff, new_stream=filtered_new_cube
        )

        not_comparable_stream, comparable_stream = diff_stream.split(
            qf.defined(NEW_VALUE, OLD_VALUE)
        )

        just_new_stream, just_old_stream = not_comparable_stream.split(
            qf.defined(OLD_VALUE)
        )

        just_new_stream.sort(NEW_VALUE).put(just_new_path)
        just_old_stream.sort(OLD_VALUE).put(just_old_path)

        diff_cube_columns = list(set(old_dimensions + new_dimensions +
                                     [OLD_VALUE, NEW_VALUE]))

        diff_stream = comparable_stream.project(
            *diff_cube_columns,
            **{
                DIFF: ne.custom(lambda diff: diff + 0.0, DIFF),
                UNSIGN_DIFF: ne.custom(lambda diff: abs(diff + 0.0), DIFF),
                RELATIVE_DIFF: ne.custom(lambda diff: diff + 0.0,
                                         RELATIVE_DIFF),
                UNSIGN_RELATIVE_DIFF: ne.custom(lambda diff: abs(diff + 0.0),
                                                RELATIVE_DIFF),
            }
        )
        diff_stream.sort(RELATIVE_DIFF, UNSIGN_DIFF).put(
            diff_path_srt_by_reldiff)
        diff_stream.sort(UNSIGN_RELATIVE_DIFF, UNSIGN_DIFF).put(
            diff_path_srt_by_ureldiff)
        diff_stream.sort(UNSIGN_DIFF, UNSIGN_RELATIVE_DIFF).put(
            diff_path_srt_by_absdiff)

        job.run()

    def print_results():
        YT_TOKEN = statinfra.config.get_config_from_env(
            'OAuth/tokens.tyaml'
        )['robot-statbox-copy']

        yt_client = yt.client.Yt(proxy='hahn', token=YT_TOKEN)

        WEB_PATH_PREFIX = "https://yt.yandex-team.ru/hahn/#page=navigation&path="

        def get_row_count(path):
            return yt_client.get_attribute(  # pylint: disable=no-member
                path, "row_count")

        def get_line(path, number):
            path = '{}[#{}]'.format(path, number)
            return list(yt_client.read_table(path))[0]  # pylint: disable=no-member

        def to_int(value):
            return int(value)

        def to_round(value):
            return round(value, 3)

        TRANSFORM_TO_READABLE = {
            OLD_VALUE: to_int,
            NEW_VALUE: to_int,
            RELATIVE_DIFF: to_round,
            DIFF: to_int,
            UNSIGN_DIFF: to_int,
            UNSIGN_RELATIVE_DIFF: to_round,
        }

        def do_nothing(value):
            return value

        def print_percentiles(path, percentiles, *columns):
            print("{}{}".format(WEB_PATH_PREFIX, path))
            lines_count = get_row_count(path)
            column_nm = len(columns) + 1

            pattern = ' | '.join(['{:>12}' for _ in range(column_nm)])
            pattern = '|{} |'.format(pattern)

            print(pattern.format('percentile', *columns))
            for pct in percentiles:
                index = int((pct + 0.0) / 100.0 * lines_count)
                index = index - 1 if index else index
                record = get_line(path, index)
                for key in columns:
                    record[key] = TRANSFORM_TO_READABLE.get(key, do_nothing)(record[key])
                print(pattern.format(pct, *[record[col] for col in columns]))

        uniq_lines_in_old_cube = get_row_count(just_old_path)
        uniq_lines_in_new_cube = get_row_count(just_new_path)
        diff_lines = get_row_count(diff_path_srt_by_ureldiff)
        lines_in_old_cube = diff_lines + uniq_lines_in_old_cube
        lines_in_new_cube = diff_lines + uniq_lines_in_new_cube

        print('lines in old cube:', lines_in_old_cube)
        print('lines in new cube:', lines_in_new_cube)
        print('lines in diff cube:', diff_lines)
        print()
        print('unique lines in old cube:', uniq_lines_in_old_cube)
        print('look here: {}{}'.format(WEB_PATH_PREFIX, just_old_path))
        print()
        print('unique lines in new cube:', uniq_lines_in_new_cube)
        print('look here: {}{}'.format(WEB_PATH_PREFIX, just_new_path))

        if lines_in_new_cube == 0:
            print('no rows with desirable timestamp in new cube')
            return
        if lines_in_old_cube == 0:
            print('no rows with desirable timestamp in old cube')
            return

        print('sorted by unsigned relative diff:')
        percentiles = [0, 50, 75, 96, 97, 98, 99, 99.5, 99.99, 100]
        print_percentiles(diff_path_srt_by_ureldiff, percentiles,
                          UNSIGN_RELATIVE_DIFF, UNSIGN_DIFF, OLD_VALUE, NEW_VALUE)
        print()
        print('sorted by unsigned absolute diff:')
        percentiles = [0, 50, 75, 96, 97, 98, 99, 99.5, 99.99, 100]
        print_percentiles(diff_path_srt_by_absdiff, percentiles,
                          UNSIGN_DIFF, UNSIGN_RELATIVE_DIFF, OLD_VALUE, NEW_VALUE)
        print()
        print('sorted by relative diff:')
        percentiles = [0, 1, 2, 10, 50, 75, 90, 95, 97, 99, 99.99, 100]
        print_percentiles(diff_path_srt_by_reldiff, percentiles,
                          RELATIVE_DIFF, UNSIGN_DIFF, OLD_VALUE, NEW_VALUE)
        print()
        print('legend:')
        print("{} : old value of measure".format(OLD_VALUE))
        print("{} : new value of measure".format(NEW_VALUE))
        print("{} : signed absolute difference old and new measures".format(DIFF))
        print(("{} : signed relative difference old and new measures"
               " (from 0 to 100)").format(RELATIVE_DIFF))
        print("{} : unsigned absolute difference old and new measures".format(UNSIGN_DIFF))
        print(("{} : unsigned relative difference old and new"
              " measures (from 0 to 100)").format(UNSIGN_RELATIVE_DIFF))

    if not params.get('just_print', False):
        calculate()

    print_results()


def download_cubes_and_create_diff(**params):
    download_cube(
        params['old_report_path'], params['old_report_proxy'], params['scale'],
        'hahn', params['old_cube_yt_path']
    )
    download_cube(
        params['new_report_path'], params['new_report_proxy'], params['scale'],
        'hahn', params['new_cube_yt_path']
    )
    create_diff(**params)


def validate_datetime(datetime_text):
    try:
        dt.datetime.strptime(datetime_text, '%Y-%m-%d')
    except ValueError:
        raise argparse.ArgumentTypeError(
            "Incorrect data format, should be YYYY-MM-DD"
        )
    return datetime_text


def expand_proxy_name(short_name):
    return {'stat': statface_client.STATFACE_PRODUCTION,
            'stat-beta': statface_client.STATFACE_BETA}[short_name]


def create_parser():
    parser = argparse.ArgumentParser(
        description='Compare two Statface report cubes')

    statface_group = parser.add_argument_group('statface')
    statface_group.add_argument(
        '-o', '--old-path', dest='old_report_path',
        required=True, help='old report path')
    statface_group.add_argument(
        '-op', '--old-proxy', dest='old_report_proxy',
        choices=['stat', 'stat-beta'], required=True)
    statface_group.add_argument(
        '-n', '--new-path', dest='new_report_path',
        required=True, help='new report path')
    statface_group.add_argument(
        '-np', '--new-proxy', dest='new_report_proxy',
        choices=['stat', 'stat-beta'], required=True)
    statface_group.add_argument(
        '-s', '--scale', choices=statface_client.STATFACE_SCALES,
        required=True)
    statface_group.add_argument(
        '-d', '--startdate', required=True, type=validate_datetime,
        help='start fielddate in format YYYY-MM-DD')
    statface_group.add_argument(
        '-ed', '--enddate', type=validate_datetime,
        help='End fielddate in format YYYY-MM-DD. Defaults to startdate')
    statface_group.add_argument(
        '-m', '--measure',
        help='Measure to compare. Required if report contains more than one measure')

    yt_group = parser.add_argument_group('yt')
    YT_PATH_PREFIX = 'yt_path_prefix'

    yt_group.add_argument(
        '-yp', '--path', dest=YT_PATH_PREFIX,
        help='yt path to locate final and intermediate results',
        default=DEFAULT_RESULT_PATH
    )
    yt_group.add_argument(
        '--old-cube', dest='old_cube_yt_path',
        default='old_cube',
        help='path to old date cube')
    yt_group.add_argument(
        '--new-cube', dest='new_cube_yt_path',
        default='new_cube',
        help='path to new date cube')

    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        '--just-diff', help="use previously downloaded data qubes",
        action='store_true', default=False
    )

    group.add_argument(
        '--just-print', help="print previously calculated result",
        action='store_true', default=False
    )
    return parser


def create_parser_and_parse_args():
    parser = create_parser()
    args = parser.parse_args()
    args = vars(args)

    params = {}
    _copy_names = (
        'old_report_path', 'new_report_path',
        'scale', 'startdate',
        'measure', 'yt_path_prefix',
    )
    for name in _copy_names:
        params[name] = args[name]

    for name in ('old_report_proxy', 'new_report_proxy'):
        params[name] = expand_proxy_name(args[name])
    if args['enddate'] is None:
        params['enddate'] = args['startdate']
    else:
        params['enddate'] = args['enddate']

    for name in ('old_cube_yt_path', 'new_cube_yt_path'):
        if not args[name].startswith('//'):
            params[name] = args['yt_path_prefix'] + '/' + args[name]
        else:
            params[name] = args[name]

    print('rd run with params', json.dumps(params, indent=2))
    if args['just_diff'] or args['just_print']:
        params['action'] = create_diff
        if args['just_print']:
            params['just_print'] = True
    else:
        params['action'] = download_cubes_and_create_diff

    print('action:', params['action'])

    return params


def main():
    params = create_parser_and_parse_args()
    action = params.pop('action')
    return action(**params)


if __name__ == '__main__':
    main()
