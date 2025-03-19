#!/usr/bin/env python

import argparse

import json
import requests

CONST_TYPE_ALL = "all"
CONST_CRIT_FAILED_CYCLES = 15

CONST_WARN_MAX_PRIMARY_LAG = 30
CONST_CRIT_MAX_PRIMARY_LAG = 240

CONST_WARN_MAX_SECONDARY_LAG = 60
CONST_CRIT_MAX_SECONDARY_LAG = 480

CONST_WARN_RES_ERROR_COUNT = 10
CONST_CRIT_RES_ERROR_COUNT = 100


def _parse_args():
    arg = argparse.ArgumentParser(description='MDB DNS Live Status Checker')

    arg.add_argument(
        '--ctype',
        required=False,
        default=CONST_TYPE_ALL,
        help='Used cluster type')

    arg.add_argument(
        '--env',
        required=False,
        default=None,
        help='Used environment')

    arg.add_argument(
        '--max-update-percent',
        type=int,
        required=False,
        default=1,
        help='Maxumum update percent level')

    return arg.parse_args()


def _dns_status(client, is_prod, live_state, live_info):
    if client not in live_state:
        return
    client_state = live_state[client]

    prim_lag = 0
    prim_failed = 0
    if 'primary' in client_state:
        prim_state = client_state['primary']
        prim_lag = prim_state["pointlagsec"]
        prim_failed = prim_state["lastfailedcycles"]
    sec_lag = 0
    sec_failed = 0
    if 'secondary' in client_state:
        sec_state = client_state['secondary']
        sec_lag = sec_state["pointlagsec"]
        sec_failed = sec_state["lastfailedcycles"]
    live_info += ["%s lag primary: %d sec, secondary: %d sec" % (client, prim_lag, sec_lag)]
    if prim_failed or sec_failed:
        live_info += ["%s failed cycles primary: %d, secondary: %d" % (client, prim_failed, sec_failed)]

    check_level = 0
    if prim_lag > CONST_CRIT_MAX_PRIMARY_LAG:
        check_level = max(check_level, is_prod + 1)
        live_info += ["%s primary lag above critical" % client]
    elif prim_lag > CONST_WARN_MAX_PRIMARY_LAG:
        check_level = max(check_level, is_prod)
        live_info += ["%s primary lag above warning" % client]
    if prim_failed > CONST_CRIT_FAILED_CYCLES:
        check_level = max(check_level, is_prod + 1)
        live_info += ["%s critical primary update falied cycles: %d" % (client, prim_failed)]
    elif prim_failed:
        check_level = max(check_level, is_prod)
        live_info += ["%s primary update falied cycles: %d" % (client, prim_failed)]

    if sec_lag > CONST_CRIT_MAX_SECONDARY_LAG:
        check_level = max(check_level, is_prod)
        live_info += ["%s secondary lag much above the allowed" % client]
    elif sec_lag > CONST_WARN_MAX_SECONDARY_LAG:
        check_level = max(check_level, is_prod)
        live_info += ["%s secondary lag above the allowed" % client]
    if sec_failed > CONST_CRIT_FAILED_CYCLES:
        check_level = max(check_level, is_prod)
        live_info += ["%s secondary update falied cycles: %d" % (client, sec_failed)]

    last_res_err = 0
    if 'lastresolveerror' in client_state:
        last_res_err = client_state['lastresolveerror']
    if last_res_err > CONST_CRIT_RES_ERROR_COUNT:
        check_level = max(check_level, is_prod + 1)
        live_info += ["%s last resolve errors above the allowed" % client]
    elif last_res_err > CONST_WARN_RES_ERROR_COUNT:
        check_level = max(check_level, is_prod)
        live_info += ["%s last resolve errors" % client]

    return check_level


def _dns_ratio(is_prod, live_state, live_info, percent_level):
    check_level = 0
    ratio = live_state.get("updprimaryratio", 0.)
    if ratio >= percent_level:
        check_level = max(check_level, is_prod + 1)
        live_info += ["attention, nonempty update primary: %.0f%%" % ratio]

    ratio = live_state.get("updsecondaryratio", 0.)
    if ratio >= percent_level:
        check_level = max(check_level, is_prod)
        live_info += ["attention, nonempty update secondary: %.0f%%" % ratio]

    return check_level


def _main():
    try:
        args = _parse_args()

        req_url = 'http://localhost:26011/v1/live'
        if args.ctype != CONST_TYPE_ALL:
            req_url = 'http://localhost:26011/v1/lives/%s/%s' % (args.ctype, args.env)
        resp = requests.get(req_url)
        if resp.status_code != 200:
            print('2;Failed request live status, code: %d, msg: %s' % (resp.status_code, resp.reason))
            return

        live_state = json.loads(resp.content)
        is_prod = 'prod' in args.env.split('-')

        check_level = 0
        live_info = []

        ac = live_state['activeclients']
        if not ac:
            live_info += ["No active clients"]
            if is_prod and args.ctype == CONST_TYPE_ALL:
                check_level = max(check_level, 1)
        else:
            live_info += ["Active clients: %d" % ac]

        for client in ('slayerdns', 'computedns'):
            check_level = max(check_level, _dns_status(client, is_prod, live_state, live_info))

        check_level = max(check_level, _dns_ratio(is_prod, live_state, live_info, args.max_update_percent))

        if check_level == 0:
            live_info = ["OK"] + live_info
        print('%d;%s' % (check_level, "; ".join(live_info)))
    except Exception as exc:
        print('2;%s, %s' % ('exception', exc))


if __name__ == '__main__':
    _main()
