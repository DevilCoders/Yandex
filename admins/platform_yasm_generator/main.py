import argparse
import json
import logging
import os
import sys
from collections import namedtuple
from functools import partial
from pathlib import Path
from typing import List, Generator

from .src.Platform import Platform
from .src.log import setup_logging
from .src.parse_config import parse_config
from .src.yasm import Yasm, YasmCheck, JugglerCheck, Interval, YasmDashboard, YasmGraph, YasmAlert, YasmGraphSignal
from .src.yasm.YasmCheck import ValueModify

setup_logging()

service = namedtuple('Service', ['project', 'application', 'environment', 'component'])


def read_template(template: Path):
    if not template.exists() and not template.is_file():
        logging.error(f"Template {template.resolve()} does not exist")
        sys.exit()

    with template.open('r') as fobj:
        return json.load(fobj)


def merge_defaults(defaults, target):
    for key, value in defaults.items():
        if key not in target:
            target[key] = value
        elif isinstance(value, dict):
            if isinstance(target[key], bool):
                continue
            merge_defaults(value, target[key])
    return target


def convert_percent_to_float(value):
    if "%" in str(value)[-1]:
        return int(value[:-1]) / 100.0
    else:
        return int(value)


signals = {
    "cpu": "div(portoinst-cpu_usage_cores_tmmv, portoinst-cpu_limit_cores_tmmv)",
    "network": "div(portoinst-net_rx_mb_summ, portoinst-net_guarantee_mb_summ)",
    "mem": "div(portoinst-anon_usage_gb_tmmv, portoinst-anon_limit_tmmv)",
    "listen_overflows": "hsum(unistat-net_tcp_listen_overflows_dhhh)",
    "upstream_errors": "or(div(push-proxy_errors_summ, push-requests_summ), 0)",
    "5xx_without_502_504": "or(div(diff(push-response_5xx_summ,sum(push-http_502_summ,push-http_504_summ)),push-requests_summ), 0)",
    "502_504": "or(div(sum(push-http_502_summ, push-http_504_summ), push-requests_summ), 0)",
    "499": "or(div(push-http_499_summ, push-requests_summ), 0)",
    "unispace": "div(conv(unistat-auto_disk_rootfs_usage_bytes_axxx, Gi), conv(unistat-auto_disk_rootfs_total_bytes_axxx, Gi))",
}


def main():
    args = parse_args()

    raw_conf = Path(f"{args.configs}/{args.project}.yaml")
    config = parse_config(raw_conf)
    defaults = config["default"]
    templates = Path("./src/templates/checks")

    if args.application:
        process_service(config, defaults, templates, args.application, args)
    else:
        for x in config.keys() - {"default", }:
            process_service(config, defaults, templates, x, args)


def column_generator():
    col = 1
    while True:
        yield col
        col += 1


def row_generator(column_count=4) -> Generator:
    row = 0
    count = 0
    while True:
        if count % column_count == 0:
            row += 1
        yield row
        count += 1


def graph_generator(graph_type, row: Generator, dcs, sv: service, balancer: bool = False) -> List[YasmGraph]:
    col = column_generator()
    title = graph_type

    if balancer:
        title = 'L7 ' + title
        yasm_tag = f"itype=qloudrouter;ctype={sv.environment};component={sv.component}"
    else:
        yasm_tag = f"itype=qloud;prj={sv.project}.{sv.application}.{sv.environment};component={sv.component}"

    signals = {
        'listen_overflows': {
            'limit': 'fake',
            'usage': 'hsum(unistat-net_tcp_listen_overflows_dhhh)',
        },
        'cpu': {
            'limit': 'portoinst-cpu_limit_cores_tmmv',
            'usage': 'portoinst-cpu_usage_cores_tmmv',
        },
        'network': {
            'limit': 'portoinst-net_guarantee_mb_summ',
            'usage': 'portoinst-net_rx_mb_summ',
        },
        'mem': {
            'limit': 'portoinst-anon_limit_tmmv',
            'usage': 'portoinst-anon_usage_gb_tmmv',
        },
        'unispace': {
            'limit': 'conv(unistat-auto_disk_rootfs_total_bytes_axxx, Gi)',
            'usage': 'conv(unistat-auto_disk_rootfs_usage_bytes_axxx, Gi)',
        },
    }
    return [
        YasmGraph(
            title=title,
            row=next(row),
            column=next(col),
            signals=[
                YasmGraphSignal(
                    title="usage",
                    tag=yasm_tag,
                    name=signals[graph_type]["usage"]),
                YasmGraphSignal(
                    title="limit",
                    tag=yasm_tag,
                    name=signals[graph_type]["limit"]),
            ]
        ),
        *[
            YasmGraph(
                title=f'{title} {dc}',
                row=next(row),
                column=next(col),
                signals=[
                    YasmGraphSignal(
                        title="usage",
                        tag=yasm_tag + f";geo={dc}",
                        name=signals[graph_type]["usage"]),
                    YasmGraphSignal(
                        title="limit",
                        tag=yasm_tag + f";geo={dc}",
                        name=signals[graph_type]["limit"]),
                ]
            ) for dc in dcs]
    ]


def upstream_graphs(row: Generator, sv: service, bl: service) -> List[YasmGraph]:
    col = column_generator()
    tag = f"itype=qloudrouter;ctype={bl.environment};prj={sv.project}.{sv.application}.{sv.environment};tier={sv.component}-*"
    signals = [
        ('upstream errors', 'or(div(push-proxy_errors_summ,push-requests_summ), 0)'),
        ('499', 'or(div(push-http_499_summ,push-requests_summ), 0)'),
        ('502 + 504', 'or(div(sum(push-http_502_summ,push-http_504_summ),push-requests_summ), 0)'),
        ('5xx - (502 + 504)', 'or(div(diff(push-response_5xx_summ,sum(push-http_502_summ,push-http_504_summ)),'
                              'push-requests_summ), 0)')
    ]
    return [
        YasmGraph(
            title=title,
            row=next(row),
            column=next(col),
            signals=[
                YasmGraphSignal(
                    title="percent",
                    tag=tag,
                    name=name),
            ]
        ) for title, name in signals
    ]


def process_service(config, defaults, templates, x, args):
    config = merge_defaults(defaults, config[x])
    sv = service(*x.split('.'))

    app_tag = f"qloud-ext.{sv.project}.{sv.application}.{sv.environment}"
    alerts = []
    dashboard = YasmDashboard(title=f"{sv.project}.{sv.application}.{sv.environment}.{sv.component}",
                              key=f"{sv.project}-{sv.application}-{sv.environment}-{sv.component}",
                              editors=config['editors'])

    row = row_generator()

    if config['balancer']:
        create_balancer_checks(alerts, app_tag, args, config, dashboard, row, sv)

    checks = ['cpu', 'mem', 'unispace', 'network']
    for c in checks:
        column = column_generator()

        dashboard.charts += graph_generator(c, row, config['dc'], sv)
        all_checks_window_modifiers = config['modify_checks']
        current_check_modify_windows = all_checks_window_modifiers.get(c, all_checks_window_modifiers['default'])
        current_check_value_modify = ValueModify(
            window=current_check_modify_windows['value_modify']['window'],
            type_=current_check_modify_windows['value_modify']['type']
            )

        yasm_check = partial(YasmCheck,
                             name=f"{sv.project}.{sv.application}.{sv.environment}.{sv.component}.{c}",
                             signal=signals[c],
                             juggler_check=JugglerCheck(
                                 namespace=sv.project,
                                 host=f"qloud-ext.{sv.project}.{sv.application}.{sv.environment}.{sv.component}",
                                 service=c,
                                 tags=config['juggler_tag'] + [app_tag]
                             ),
                             value_modify=current_check_value_modify,
                             tags={
                                 "itype": ['qloud'],
                                 "prj": [f"{sv.project}.{sv.application}.{sv.environment}"],
                                 "component": [sv.component],
                             },
                             crit=Interval(from_=convert_percent_to_float(config['app'][c]))
                             )

        dashboard.charts.append(
            YasmAlert(
                row=next(row),
                title=c,
                name=f"{sv.project}.{sv.application}.{sv.environment}.{sv.component}.{c}",
                column=next(column)
            )
        )

        alerts.append(yasm_check())

        for dc in config['dc']:
            dashboard.charts.append(
                YasmAlert(
                    row=next(row),
                    title=f"{c} {dc}",
                    name=f"{sv.project}.{sv.application}.{sv.environment}.{sv.component}.{c}_{dc}",
                    column=next(column)
                )
            )

            all_checks_window_modifiers = config['modify_checks']
            current_check_modify_windows = all_checks_window_modifiers.get(f"{c}_{dc}", all_checks_window_modifiers['default'])
            current_check_value_modify = ValueModify(
                window=current_check_modify_windows['value_modify']['window'],
                type_=current_check_modify_windows['value_modify']['type']
                )

            alerts.append(yasm_check(
                tags={
                    "itype": ['qloud'],
                    "prj": [f"{sv.project}.{sv.application}.{sv.environment}"],
                    "component": [sv.component],
                    "geo": dc,
                },
                name=f"{sv.project}.{sv.application}.{sv.environment}.{sv.component}.{c}_{dc}",
                value_modify=current_check_value_modify,
                juggler_check=JugglerCheck(
                    namespace=sv.project,
                    host=f"qloud-ext.{sv.project}.{sv.application}.{sv.environment}.{sv.component}",
                    service=f"{c}_{dc}",
                    tags=config['juggler_tag'] + [app_tag]
                ),
            ))

    if config['balancer']:
        create_balancer_res_checks(alerts, app_tag, args, config, dashboard, row, sv)

    yasm = Yasm()
    for alert in alerts:
        if args.delete:
            logging.info(f"Deleting {alert.name}")
            yasm.delete_alert(alert)
            continue
        if yasm.get_alert(alert):
            yasm.update_alert(alert)
        else:
            yasm.create_alert(alert)

    user = sv.project
    if args.delete:
        logging.info(f"Deleting balancer {user}.{dashboard.key}")
        yasm.delete_panel(dashboard.key, user)
    else:
        yasm.create_panel(dashboard, user)


def create_balancer_res_checks(alerts, app_tag, args, config, dashboard, row: Generator, sv: service):
    logging.info("Creating balancer res checks")
    pl = Platform(project=sv.project, token=args.token)
    bl = pl.balancer(config['balancer']['name'])
    env = pl.environment(bl.environmentId)
    balancer = service(*env.objectId.split('.'), list(env.components.keys())[0])

    balancer_tag = f"qloud-ext.{balancer.project}.{balancer.application}.{balancer.environment}"
    checks = ["cpu", "network", "listen_overflows"]
    for check in checks:
        dashboard.charts += graph_generator(check, row, config['dc'], balancer, True)

        column = column_generator()
        dashboard.charts.append(
            YasmAlert(
                row=next(row),
                title=f"L7 {check}",
                name=f"{balancer.project}.{balancer.application}.{balancer.environment}.{check}",
                column=next(column)
            )
        )
        all_checks_window_modifiers = config['modify_checks']
        current_check_modify_windows = all_checks_window_modifiers.get(check, all_checks_window_modifiers['default'])
        current_check_value_modify = ValueModify(
            window=current_check_modify_windows['value_modify']['window'],
            type_=current_check_modify_windows['value_modify']['type']
            )
        yasm_check = partial(YasmCheck,
                             name=f"{balancer.project}.{balancer.application}.{balancer.environment}.{check}",
                             signal=signals[check],
                             juggler_check=JugglerCheck(
                                 namespace=sv.project,
                                 host=f"qloud-ext.{balancer.project}.{balancer.application}.{balancer.environment}.{balancer.component}",
                                 service=check,
                                 tags=config['juggler_tag'] + [balancer_tag]
                             ),
                             value_modify=current_check_value_modify,
                             tags={
                                 "itype": ['qloudrouter'],
                                 "ctype": [f"{balancer.environment}"],
                                 "component": [f"{balancer.component}"],
                             },
                             crit=Interval(from_=convert_percent_to_float(config['balancer'][check]))
                             )
        alerts.append(yasm_check())
        for dc in config['dc']:
            dashboard.charts.append(
                YasmAlert(
                    row=next(row),
                    title=f"L7 {check} {dc}",
                    name=f"{balancer.project}.{balancer.application}.{balancer.environment}.{check}_{dc}",
                    column=next(column)
                )
            )

            all_checks_window_modifiers = config['modify_checks']
            current_check_modify_windows = all_checks_window_modifiers.get(f"{check}_{dc}", all_checks_window_modifiers['default'])
            current_check_value_modify = ValueModify(
                window=current_check_modify_windows['value_modify']['window'],
                type_=current_check_modify_windows['value_modify']['type']
                )
            alerts.append(yasm_check(
                value_modify=current_check_value_modify,
                tags={
                    "itype": ['qloudrouter'],
                    "ctype": [f"{balancer.environment}"],
                    "component": [f"{balancer.component}"],
                    "geo": dc,
                },
                name=f"{balancer.project}.{balancer.application}.{balancer.environment}.{check}_{dc}",
                juggler_check=JugglerCheck(
                    namespace=sv.project,
                    host=f"qloud-ext.{balancer.project}.{balancer.application}.{balancer.environment}.{balancer.component}",
                    service=f"{check}_{dc}",
                    tags=config['juggler_tag'] + [app_tag]
                ),
            ))


def create_balancer_checks(alerts, app_tag, args, config, dashboard, row: Generator, sv: service):
    logging.info("Creating balancer checks")
    pl = Platform(project=sv.project, token=args.token)
    bl = pl.balancer(config['balancer']['name'])
    env = pl.environment(bl.environmentId)
    balancer = service(*env.objectId.split('.'), list(env.components.keys())[0])
    dashboard.charts += upstream_graphs(row, sv, balancer)
    checks = {'upstream_errors': 'upstream_errors',
              '499': 499,
              '502_504': "502+504",
              '5xx_without_502_504': "5xx-(502+504)"}
    column = column_generator()
    for check, v in checks.items():
        dashboard.charts.append(
            YasmAlert(
                row=next(row),
                title=f"{check}",
                name=f"{sv.project}.{sv.application}.{sv.environment}.{sv.component}.{check}",
                column=next(column)
            )
        )
        all_checks_window_modifiers = config['modify_checks']
        current_check_modify_windows = all_checks_window_modifiers.get(v, all_checks_window_modifiers['default'])
        current_check_value_modify = ValueModify(
            window=current_check_modify_windows['value_modify']['window'],
            type_=current_check_modify_windows['value_modify']['type']
            )
        yasm_check = partial(YasmCheck,
                             name=f"{sv.project}.{sv.application}.{sv.environment}.{sv.component}.{check}",
                             signal=signals[check],
                             juggler_check=JugglerCheck(
                                 namespace=sv.project,
                                 host=f"qloud-ext.{sv.project}.{sv.application}.{sv.environment}.{sv.component}",
                                 service=check,
                                 tags=config['juggler_tag'] + [app_tag]
                             ),
                             value_modify=current_check_value_modify,
                             tags={
                                 "itype": ['qloudrouter'],
                                 "prj": [f"{sv.project}.{sv.application}.{sv.environment}"],
                                 "ctype": [f"{balancer.environment}"],
                                 "tier": [f"{sv.component}-*"],
                             },
                             crit=Interval(from_=convert_percent_to_float(config['balancer'][v]))
                             )
        alerts.append(yasm_check())


def parse_args():
    parser = argparse.ArgumentParser(description="This tools create juggler checks and yasm panel",
                                     usage="\n./yasm_dashboard -p kp  -a auth\n"
                                           "'auth' is an application name",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("-t", "--token", help="Platform OAuth token. Default from $PLATFORM_TOKEN env variable")
    parser.add_argument("--configs", help="Recommended config values", default="./configs")
    parser.add_argument("-p", "--project", help="Platform project", required=True)
    parser.add_argument("-a", "--application", help="Project application")
    parser.add_argument("--user", help="Yasm dashboard user", default=os.environ['USER'])
    parser.add_argument("-u", "--update", help="Project application", action="store_true")

    parser.add_argument("-d", "--delete", help="Remove selected checks and yasm panels", action="store_true")

    return parser.parse_args()


if __name__ == '__main__':
    main()
