# -*- coding: utf-8 -*-

import argparse
import pathlib
import colorama
import datetime

from cloud.iam.planning_tool.library.config import Config
from cloud.iam.planning_tool.library.daily import DailyReport
from cloud.iam.planning_tool.library.monthly import MonthlyReport
from cloud.iam.planning_tool.library.tasks import TaskReport
from cloud.iam.planning_tool.library.tax import TaxReport
from cloud.iam.planning_tool.library.timesheet import Timesheet
from cloud.iam.planning_tool.library.tools import PlanningTools
from cloud.iam.planning_tool.library.utils import styled_print, with_mock, with_log
from cloud.iam.planning_tool.library.report import UtilBuilder


def get_parser():
    parser = argparse.ArgumentParser()
    base_parser = argparse.ArgumentParser()
    base_parser.add_argument('-c', '--config', help='config file path', default='configs/iam.yaml')
    base_parser.add_argument('-b', '--beautify', help='automatically beautifying output html', action='store_true')
    base_parser.add_argument('-tp', '--template',
                             help='path for template to use (should in template directory if relative')
    dev_tools_group = base_parser.add_argument_group(title='devtools', description='tools for development reasons')
    dev_tools_group.add_argument('-ro', '--raw-output', help='raw output path')
    log_tool = dev_tools_group.add_mutually_exclusive_group()
    log_tool.add_argument('-l', '--log', help='log all responses', action='store_true')
    log_tool.add_argument('-lf', '--log-file', help='file for writing requests logs', default='')
    mock_tool = dev_tools_group.add_mutually_exclusive_group()
    mock_tool.add_argument('-m', '--mock', help='mock all requests', action='store_true')
    mock_tool.add_argument('-mf', '--mock-file', help='file with requests logs', default='')
    mds_saving_group = base_parser.add_mutually_exclusive_group()
    mds_saving_group.add_argument('--mds-on', action='store_true',
                                  help='upload report to the mds (have priority over config)')
    mds_saving_group.add_argument('--mds-off', action='store_true',
                                  help='do not upload report to the mds (have priority over config)')

    local_saving_group = base_parser.add_mutually_exclusive_group()
    local_saving_group.add_argument('-o', '--output', help='output path (replaces path from config')
    local_saving_group.add_argument('--output-off', action='store_true', help='disabling local saving')

    subparsers = parser.add_subparsers(help='sub-command help', dest='command')

    parser_tools = subparsers.add_parser('tools', add_help=False, parents=[base_parser])
    parser_tools.add_argument('-i', '--issues', action='store_true', help='check issues')

    parser_daily = subparsers.add_parser('daily', add_help=False, parents=[base_parser])
    parser_daily.add_argument('-d', '--date', help='report date, e.g. 2019-11-17')
    parser_daily.add_argument('-s', '--shift', action='store_true', help='shift list of person')
    parser_daily.add_argument('-r', '--release', action='store_true', help='make a release page /{date}')

    parser_monthly = subparsers.add_parser('monthly', add_help=False, parents=[base_parser])
    parser_monthly.add_argument('-f', '--from', dest='from_date', required=True,
                                help='from date (inclusively), e.g. 2019-11-17')
    parser_monthly.add_argument('-t', '--to', help='to date (exclusively), e.g. 2020-01-31')
    parser_monthly.add_argument('-n', '--new-tag', help='')
    parser_monthly.add_argument('-p', '--previous-tag', help='')
    parser_monthly.add_argument('-r', '--release', action='store_true', help='make a release page /{date}')

    parser_tax = subparsers.add_parser('tax', add_help=False, parents=[base_parser])
    parser_tax.add_argument('-f', '--from', dest='from_date', required=True,
                            help='from date (inclusively), e.g. 2019-11-17')
    parser_tax.add_argument('-t', '--to', help='to date (exclusively), e.g. 2020-01-31')
    parser_tax.add_argument('-p', '--path', help='make a release page', required=True)
    parser_tax.add_argument('-i', '--title', help='make a release page', required=True)

    parser_timesheet = subparsers.add_parser('timesheet', add_help=False, parents=[base_parser])
    parser_timesheet.add_argument('-f', '--from', dest='from_date', required=True,
                                  help='from date (inclusively), e.g. 2019-11-17')
    parser_timesheet.add_argument('-t', '--to', help='to date (exclusively), e.g. 2020-01-31')
    parser_timesheet.add_argument('-r', '--release')

    parser_tasks = subparsers.add_parser('tasks', add_help=False, parents=[base_parser])
    parser_tasks.add_argument('-f', '--from', dest='created_from_date',
                              help='created from date (inclusively), e.g. 2019-11-17')
    parser_tasks.add_argument('-cl', '--show-closed', action='store_true', help='show closed tasks')
    parser_tasks.add_argument('-r', '--release', action='store_true', help='make a release page /{date}')

    parser_raw = subparsers.add_parser('raw', add_help=False, parents=[base_parser])
    parser_raw.add_argument('-f', '--file', help='file with raw report data')

    return parser


def main():
    colorama.init()
    parser = get_parser()

    args = parser.parse_args()

    template_path = None
    template_name = None
    if args.template:
        template_path = pathlib.Path(args.template)
        template_name = template_path.name
        template_path = pathlib.Path.joinpath(pathlib.Path('' if template_path.is_absolute() else 'templates'),
                                              template_path.parent)
    # по умолчанию считаем, что mds ключ нужен
    config = Config(args.config, not args.mds_off, template_path=template_path)
    styled_print(f'Using config from {args.config} :')
    print(config.dump())

    call_func = prepare_util
    if args.log or args.log_file != '':
        if args.log_file == '':
            args.log_file = 'log.json'
        call_func = with_log(args.log_file)(call_func)
    if args.mock or args.mock_file != '':
        if args.mock_file == '':
            args.mock_file = 'log.json'
        call_func = with_mock(args.mock_file)(call_func)

    util, release_date = call_func(args, config)
    util = util.build()
    util.render(template_name)
    if args.beautify and args.command != 'tax':
        util.beautify()
    if output := (not args.output_off and (args.output or config.local_output)):
        util.save(output)
    if args.raw_output:
        util.save_prepared(args.raw_output)

    if args.command == 'tax':
        config.wiki.upload(args.path, args.title, util.rendered)
    elif config.mds_saving:
        util.upload(release_date=release_date if args.release else None)


def prepare_util(args, config) -> tuple[UtilBuilder, datetime.datetime]:
    util = UtilBuilder(config)
    if args.command == 'daily':
        to_date = datetime.date.fromisoformat(args.date) if args.date else datetime.date.today()

        print()
        styled_print(f'Generating Daily Report, date: {to_date}')
        util.set_report_cls(DailyReport)
        util.prepare_from_web(to_date, args.shift)
    elif args.command == 'monthly':
        from_date = datetime.date.fromisoformat(args.from_date)
        to_date = datetime.date.fromisoformat(args.to) if args.to else datetime.date.today()

        print()
        styled_print(f'Generating Monthly Report, from date: {from_date}, to date: {to_date}')

        util.set_report_cls(MonthlyReport)
        util.prepare_from_web(from_date, to_date, args.new_tag, args.previous_tag)
    elif args.command == 'tax':
        from_date = datetime.date.fromisoformat(args.from_date)
        to_date = datetime.date.fromisoformat(args.to) if args.to else datetime.date.today()

        print()
        styled_print(f'Generating Tax Report, from date: {from_date}, to date: {to_date}')

        util.set_report_cls(TaxReport)
        util.prepare_from_web(from_date, to_date)
    elif args.command == 'tasks':
        from_date = None
        if args.created_from_date is not None:
            from_date = datetime.date.fromisoformat(args.created_from_date)

        print()
        styled_print(f'Generating Tasks Report, created from date: {from_date},'
                     f' show closed: {args.show_closed}')

        util.set_report_cls(TaskReport)
        util.prepare_from_web(from_date, args.show_closed)
        to_date = datetime.date.today()
    elif args.command == 'timesheet':
        from_date = datetime.date.fromisoformat(args.from_date)
        to_date = datetime.date.fromisoformat(args.to) if args.to else datetime.date.today()

        print()
        styled_print(f'Generating Timesheet Report, from date: {from_date}, to date {to_date}')

        util.set_report_cls(Timesheet)
        util.prepare_from_web(from_date, to_date)
    elif args.command == 'tools':
        PlanningTools(config).check_issues()
        return
    elif args.command == 'raw':
        to_date = datetime.date.today()
        util.prepare_from_file(args.file)
    else:
        raise RuntimeError(f'Unknown command {args.command}')

    return util, to_date


if __name__ == '__main__':
    main()
