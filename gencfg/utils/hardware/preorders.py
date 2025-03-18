#!/usr/bin/env python
# coding=utf-8

import os
import requests
import sys

from collections import defaultdict

from servers import _load_table_from_string
from tools import dump_json

import resources_counter


def _get_orders_from_args_or_stdin(orders):
    if not orders:
        orders = sys.stdin.read()
    else:
        orders = " ".join(orders)

    orders = sorted(set([el.split("/")[-1] for el in orders.replace(",", " ").split()]))

    return orders


def bot_token():
    return file(os.path.expanduser("~/.bot/token")).read().strip()


def get_preorder_stat(order):
    order_url_base = "https://bot.yandex-team.ru/api/v1/hwr/pre-orders/%s"

    token = bot_token()

    url = order_url_base % order

    res = requests.get(
        url,
        headers={'Authorization': 'OAuth {}'.format(token)}
    )
    data = res.json()
    if res.status_code != 200:
        raise Exception("API error on %s: %s\n" % (url, data.get("message")))

    current = data["current_version"]

    return {
        "name": current["big_order_config"]["full_name"],
        "dc": current["big_order_config"]["location"]["name"].lower(),
        "configuration": current["server"]["full_name"],
        "servers_count": current["servers_quantity"],
    }


def preorders_stat(orders):
    for order in _get_orders_from_args_or_stdin(orders):
        data = get_preorder_stat(order)

        sys.stdout.write("https://bot.yandex-team.ru/hwr/preorders/%s: %s %s x %s\n" % (
            order, data["name"], data["servers_count"], data["configuration"]))


def evaluate_bot_orders(config):
    res = resources_counter.ResourcesCounter()

    for order in config.get("bot", {}).get("orders", []):
        for preorder in order["preorders"]:
            temp = resources_counter.empty_resources()
            temp["cloud"] = "bot"

            order_data = get_preorder_stat(preorder)
            temp["dc"] = order_data["dc"]
            servers_count = order_data["servers_count"]

            for key, val in order["server_configuration"].iteritems():
                temp[key] = val * servers_count

            res.add_resource(temp)

    return res


def preorders_to_inv(orders, full=False):
    url_base = "https://bot.yandex-team.ru/api/preorders.php?id=%s"

    orders = _get_orders_from_args_or_stdin(orders)
    sys.stderr.write("Orders: %s\n\n" % ", ".join(orders))

    for order in sorted(orders):
        url = url_base % order
        data = requests.get(url).json()
        status = data.get("info", {}).get("status")

        if status != 'CLOSE':
            sys.stderr.write("order %s is not closed (%s)\n" % (order, status))

        count = int(data["info"]["count"])

        servers = [server["inv"] for server in data["servers"]]
        if len(servers) < count:
            sys.stderr.write("order %s is not complete (%s / %s)\n" % (order, len(servers), count))

        for server in sorted(servers):
            if full:
                sys.stdout.write("%s\t%s\n" % (order, server))
            else:
                sys.stdout.write("%s\n" % server)


def hosts_to_orders():
    url_base = "https://bot.yandex-team.ru/api/v1/hwr/setup/servers/%s"

    token = file(os.path.expanduser("~/.bot/token")).read().strip()

    orders = defaultdict(list)
    for inv in sys.stdin.read().split():
        data = requests.get(
            url_base % inv,
            headers={'Authorization': 'OAuth {}'.format(token)}
        )

        if data.status_code != 200:
            order = "unknown"
        else:
            order = data.json()["pre_order_id"]

        orders[order].append(inv)

    for order, invs in sorted(orders.iteritems()):
        if order != "unknown":
            order = "https://bot.yandex-team.ru/hwr/preorders/%s" % order
        sys.stdout.write("%s\t%s\n" % (order, len(invs)))


def _load_configurations_power(path="hwr_allocation__xls_table_2020-09-18_19-56.csv"):
    table = file(path).read().decode("utf-8-sig").encode("utf-8")
    table = _load_table_from_string(table, delimiter=",")

    configurations_power = {}
    configurations_preorders = defaultdict(list)
    for row in table:
        preorder = row["preorder_id"]
        servers_configuration = row["server_name"]
        abc_service = row["service_ru"]

        configuration = " | ".join([abc_service, servers_configuration])

        configurations_preorders[configuration].append(preorder)

        configuration_power = int(row["forecasted_server_pwr"])

        prev_configuration_power = configurations_power.get(configuration)
        if prev_configuration_power is not None and prev_configuration_power != configuration_power:
            sys.stderr.write("configuration_power missmatch for %s: %s vs %s\n" % (
                configuration, prev_configuration_power, configuration_power))
            sys.stderr.write("preorders %s\n" % ", ".join(sorted(configurations_preorders[configuration])))
            continue

        configurations_power[configuration] = configuration_power

    return configurations_power


def power_allocation():
    configurations_power = _load_configurations_power()
    dump_json(configurations_power, "configurations_power.json")  # DEBUG

    orders_url = "https://bot.yandex-team.ru/api/v1/hwr/pre-orders?filter[versions][current][]=1&filter[versions][big_order_config][id][]=250&filter[versions][big_order_config][id][]=251&filter[versions][big_order_config][id][]=252&filter[versions][big_order_config][id][]=253&filter[versions][big_order_config][id][]=254&filter[versions][big_order_config][id][]=255&filter[versions][big_order_config][id][]=256&filter[versions][big_order_config][id][]=257&filter[versions][big_order_config][id][]=260&filter[versions][big_order_config][id][]=242&filter[versions][big_order_config][id][]=243&filter[versions][big_order_config][id][]=244&filter[versions][big_order_config][id][]=245&filter[versions][big_order_config][id][]=246&filter[versions][big_order_config][id][]=247&filter[versions][big_order_config][id][]=248&filter[versions][big_order_config][id][]=249&filter[versions][big_order_config][id][]=259&filter[versions][big_order_config][id][]=234&filter[versions][big_order_config][id][]=235&filter[versions][big_order_config][id][]=236&filter[versions][big_order_config][id][]=237&filter[versions][big_order_config][id][]=238&filter[versions][big_order_config][id][]=239&filter[versions][big_order_config][id][]=240&filter[versions][big_order_config][id][]=241&filter[versions][big_order_config][id][]=258"

    data = requests.get(orders_url, headers={'Authorization': 'OAuth {}'.format(bot_token())}).json()
    dump_json(data, "preorders.json")  # DEBUG

    dc_orders_power = defaultdict(int)

    for preorder in data["results"]:
        preorder_id = preorder["id"]

        current = preorder["current_version"]
        if not current["server"]:
            continue
        servers_configuration = current["server"]["full_name"]
        servers_count = current["servers_quantity"]

        dc = current["big_order_config"]["location"]["name"]
        order_full_name = current["big_order_config"]["full_name"]

        abc_service = current["oebs_service"]["ru_description"]

        configuration = " | ".join([abc_service, servers_configuration]).encode("utf-8")

        server_power = configurations_power.get(configuration)

        if server_power is None:
            sys.stderr.write("Can't find power for order %s: %s\n" % (preorder_id, configuration))
            continue

        dc_orders_power[order_full_name] += server_power * servers_count

    for key, val in dc_orders_power.iteritems():
        print key, val


def main():
    from optparse import OptionParser

    parser = OptionParser()

    parser.add_option("--stat", action="store_true")
    parser.add_option("--preorders", action="store_true")
    parser.add_option("--full", action="store_true")
    parser.add_option("--hosts-to-orders", action="store_true")
    parser.add_option("--power-allocation", action="store_true")
    parser.add_option("--test", action="store_true")

    (options, args) = parser.parse_args()

    if options.preorders:
        preorders_to_inv(args, full=options.full)
    if options.stat:
        preorders_stat(args)
    if options.hosts_to_orders:
        hosts_to_orders()
    if options.power_allocation:
        power_allocation()


if __name__ == "__main__":
    main()
