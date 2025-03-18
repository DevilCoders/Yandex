#!/usr/bin/env python
# coding=utf-8

import copy
import json
import os
import pickle
import requests
import sys

from collections import OrderedDict, defaultdict

import resources_counter
import startrek
import tools

import urllib3

urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

u"""
https://wiki.yandex-team.ru/intranet/abc/dispenser/api/

- Провайдеры ресурсов: https://dispenser.test.yandex-team.ru/common/api/v1/services/all (https://wiki.yandex-team.ru/dispenser/api/#get-services)
- Ресурсы провайдера: https://dispenser.test.yandex-team.ru/common/api/v1/services/yp/resources (https://wiki.yandex-team.ru/dispenser/api/#get-resources)
- Сегментации ресурса: https://dispenser.test.yandex-team.ru/common/api/v1/services/yp/resources/cpu/segmentations
- Cегменты сегментации: https://dispenser.test.yandex-team.ru/common/api/v1/segmentations/locations/segments
"""


def get_dispenser_token():
    token_path = "~/.dispenser/token"
    abs_token_path = os.path.expanduser(token_path)
    if not os.path.exists(abs_token_path):
        print "Dispenser OAuth key is absent: %s" % token_path
        print "Read more at https://wiki.yandex-team.ru/dispenser/api/"
        exit(1)
    key = file(abs_token_path).read().strip()
    return key


def get_providers():
    url = "https://dispenser.yandex-team.ru/common/api/v1/services/all"
    response = requests.get(url, verify=False, headers={'Authorization': 'OAuth {}'.format(get_dispenser_token())})
    if response.status_code != 200:
        raise Exception("Dispenser error: %s\n\n" % response.json())

    return response.json()


def orders():
    url = "https://dispenser.yandex-team.ru/common/api/v1/quota-requests/"
    config = resources_counter.load_config()
    runtime_clouds = map(str, config["orders"]["runtime_clouds"])
    params = {
        "service": runtime_clouds
    }
    response = requests.get(url, verify=False, params=params, headers={'Authorization': 'OAuth {}'.format(get_dispenser_token())})
    if response.status_code != 200:
        raise Exception("Dispenser error: %s\n\n" % response.json())

    return response.json()


def load_orders(reload=False, debug_dump=False):
    res_pickle_path = os.path.join(resources_counter.create_resources_dumps_dir(), "dispenser_orders.pickle")
    if not reload and os.path.exists(res_pickle_path):
        with file(res_pickle_path) as f:
            data = pickle.load(f)
    else:
        data = orders()

        with file(res_pickle_path, "wb") as f:
            pickle.dump(data, f)

    if debug_dump:
        debug_json_path = os.path.join(resources_counter.create_resources_dumps_dir(), "dispenser_orders.json")
        tools.dump_json(data, debug_json_path)

    return data


def _parse_resource(amount, resource_name, ticket):
    value = amount["value"]

    unit = amount["unit"]
    if unit == "BYTE":
        if resource_name == "mem":
            value /= 1024. ** 3
        elif resource_name in ["hdd", "ssd"]:
            value /= 1024. ** 4
        else:
            assert False
    elif unit == "PERMILLE_CORES":
        value /= 1000
    elif unit == "PERMILLE" and resource_name == "gpu":
        value /= 1000
    elif unit == "BINARY_BPS":
        assert resource_name in ["hdd_io", "ssd_io"]
        value /= 1024. ** 2
    else:
        sys.stderr.write("unknown unit %s at %s\n" % (unit, ticket))

    return value


def _parse_changes(changes, ticket, set_segment=None, clouds=None, order_dates=None, set_cloud="order"):
    resources_remaining = resources_counter.ResourcesCounter()
    resources_ready = resources_counter.ResourcesCounter()

    """
    {
        "amount": {
            "humanized": {
                "abbreviation": "B",
                "doubleValue": 0.0,
                "stringValue": "0",
                "unit": "BYTE"
            },
            "unit": "BYTE",
            "value": 0
        },
        "amountAllocated": {
            "humanized": {
                "abbreviation": "B",
                "doubleValue": 0.0,
                "stringValue": "0",
                "unit": "BYTE"
            },
            "unit": "BYTE",
            "value": 0
        },
        "amountReady": {
            "humanized": {
                "abbreviation": "B",
                "doubleValue": 0.0,
                "stringValue": "0",
                "unit": "BYTE"
            },
            "unit": "BYTE",
            "value": 0
        },
        "order": {
            "id": 41,
            "orderDate": "2020-11-30",
            "valid": true
        },
        "resource": {
            "key": "ssd",
            "name": "SSD"
        },
        "segmentKeys": [
            "MAN"
        ],
        "service": {
            "key": "yp",
            "name": "YP"
        }
    }
    """

    names_map = {
        "ram": "mem",
        "cpu_segmented": "cpu",
        "ram_segmented": "mem",
        "hdd_segmented": "hdd",
        "ssd_segmented": "ssd",
        "gpu_segmented": "gpu",
        "io_hdd": "hdd_io",
        "io_ssd": "ssd_io",
    }

    for data in changes:
        service_name = data["service"]["key"]
        if clouds and service_name not in clouds:
            continue

        if order_dates and not order_dates.get(data["order"]["orderDate"]):
            continue

        resource_name = data["resource"]["key"]
        resource_name = names_map.get(resource_name, resource_name)
        if resource_name not in resources_counter.RESOURCES_NAMES:
            if not (resource_name == "storage" and service_name == "saas"):
                sys.stderr.write("unknown resource name %s at %s\n" % (resource_name, ticket))
            continue

        segment = data["segmentKeys"]
        dc = [el.lower() for el in segment if el.lower() in resources_counter.DC_LIST]
        if len(dc) != 1:
            sys.stderr.write("unknown DC %s at %s: %s\n" % (dc, ticket, segment))
            continue
        dc = dc[0]

        value_full = _parse_resource(data["amount"], resource_name, ticket)
        value_allocated = _parse_resource(data["amountAllocated"], resource_name, ticket)
        value_ready = _parse_resource(data["amountReady"], resource_name, ticket)
        value_remaining = value_full - value_allocated
        if value_remaining < 0:
            sys.stderr.write("Negative %s remaining at %s\n" % (resource_name, ticket))
            value_remaining = 0

        if value_remaining:
            resource = resources_counter.empty_resources()
            resource["cloud"] = set_cloud if set_cloud else service_name
            resource["dc"] = dc
            resource["segment"] = set_segment

            resource[resource_name] = value_remaining

            resources_remaining.add_resource(resource)

        value_ready_remaining = max(value_ready - value_allocated, 0)
        if value_ready_remaining:
            resource = resources_counter.empty_resources()
            resource["cloud"] = "orders"
            resource["dc"] = dc
            resource["segment"] = set_segment

            resource[resource_name] = value_ready_remaining

            resources_ready.add_resource(resource)

    return resources_remaining, resources_ready


def send_message(message, ticket):
    task = startrek.get_issue(ticket)

    cc = task.followers
    cc.append(task.createdBy)
    cc = [el for el in cc if el.login != "sereglond"]
    cc = sorted(set(cc))

    task.comments.create(text=message, summonees=cc)


def get_dispenser_orders_resources_data(config, reload=False, all_campaigns=False, res_key=None, data=None,
                                        runtime_only=False):
    if data is None:
        data = load_orders(reload=reload)

    runtime_clouds = config["orders"]["runtime_clouds"]
    if not runtime_only:
        runtime_clouds += config["orders"]["extra_clouds"]

    dispenser_campains = config["orders"]["dispenser_campains"]
    order_dates = config["orders"].get("order_dates")

    orders_remaining = defaultdict(resources_counter.ResourcesCounter)
    orders_ready = defaultdict(resources_counter.ResourcesCounter)

    for order in data["result"]:
        if order["status"] in ["CANCELLED", "REJECTED"]:
            continue

        order_task = order["trackerIssueKey"]
        abc_service = order["project"]["key"]

        campaign = order["campaign"]["key"]
        use_campaign = dispenser_campains.get(campaign)
        if use_campaign is None:
            sys.stderr.write("Unknown dispenser campaign %s at %s\n" % (campaign, order_task))

        if not use_campaign and not all_campaigns:
            continue

        resources_remaining, resources_ready = _parse_changes(
            order["changes"],
            ticket=order_task,
            clouds=runtime_clouds,
            order_dates=order_dates
        )

        if res_key == "abc":
            key = abc_service
        else:
            key = order_task

        if resources_remaining:
            orders_remaining[key] += resources_remaining
        if resources_ready:
            orders_ready[key] += resources_ready

    return orders_remaining, orders_ready


def dump_table(path, rows):
    header = []
    for row in rows:
        for key in row.keys():
            if key not in header:
                header.append(key)

    out = file(path, "wb")
    out.write("%s\n" % ",".join(header))
    for row in rows:
        temp = [unicode(row.get(key, " ")) for key in header]
        out.write(("%s\n" % ",".join(temp)).encode("utf-8"))

    print path
    print


def _tickets_to_table(tickets_data):
    rows = []

    for ticket_data in tickets_data:
        row = OrderedDict()

        row["dc"] = ticket_data["dc"]
        row["provider"] = ticket_data["provider"]
        row["service"] = ticket_data["service"]
        row["ticket"] = ticket_data["ticket"]

        resources = ticket_data["resources"].total()
        if not resources:
            continue

        for resource in resources_counter.RESOURCES_NAMES:
            if resource in resources:
                row[resource] = resources[resource]

        row["hosts"] = resources_counter.resources_to_hosts(resources)
        cpu_hosts = resources_counter.resource_to_hosts("cpu", resources.get("cpu", 0))
        for resource in resources_counter.RESOURCES_NAMES:
            if resource in resources:
                hosts_from_resource = resources_counter.resource_to_hosts(resource, resources[resource])
                row["hosts(%s)/hosts(cpu)" % resource] = "%s" % (hosts_from_resource / cpu_hosts if cpu_hosts else 0)

        rows.append(row)

    return rows


def _format_dominant_order(resources):
    resources_hosts = {}

    for resource in resources_counter.RESOURCES_NAMES:
        resources_hosts[resource] = resources_counter.resource_to_hosts(resource, resources.get(resource, 0))

    max_hosts = max(resources_hosts.values())
    res = []
    for resource, hosts in sorted(resources_hosts.iteritems(), key=lambda x: -x[1]):
        res.append("%s %s%%" % (resource, hosts * 100 / max_hosts if max_hosts else 0))

    return ", ".join(res)


def create_summmary(config_path=None, debug=False, hwr=False):
    config = json.load(file(config_path)) if config_path else {}

    tickets_data = []

    total_resources = resources_counter.ResourcesCounter()

    runtime_clouds = config["orders"]["runtime_clouds"]
    dispenser_campains = config["orders"]["dispenser_campains"]
    chosen_campaign = [key for key, val in dispenser_campains.iteritems() if val]
    assert len(chosen_campaign) == 1
    chosen_campaign = chosen_campaign[0]

    data = load_orders(reload=not debug, debug_dump=debug)
    for order in data["result"]:
        if order["status"] in ["CANCELLED", "REJECTED"]:
            continue

        task = order["trackerIssueKey"]

        campaign = order["campaign"]["key"]
        use_campaign = dispenser_campains.get(campaign)
        if use_campaign is None:
            sys.stderr.write("Unknown dispenser campaign %s at %s\n" % (campaign, task))

        if not use_campaign:
            continue

        abc_service = order["project"]["key"]

        resources_remaining, resources_ready = _parse_changes(
            order["changes"],
            ticket=task,
            clouds=runtime_clouds,
            set_cloud=None
        )

        for resource in resources_remaining.resources.itervalues():
            cloud = resource["cloud"]

            resource_row = {}

            resource_row["ticket"] = task
            resource_row["provider"] = cloud
            resource_row["service"] = abc_service
            resource_row["dc"] = resource["dc"]

            for res_name in resources_counter.RESOURCES_NAMES:
                resource_row[res_name] = resource.get(res_name, 0)

            row_resource = resources_counter.ResourcesCounter()
            row_resource.add_resource(resource)
            resource_row["resources"] = row_resource

            total_resources.add_resource(resource)

            tickets_data.append(resource_row)

    tickets_rows = _tickets_to_table(tickets_data)

    dump_table(path="orders.csv", rows=tickets_rows)

    evaluate_bot_order(total_resources, campaign=chosen_campaign, hwr=hwr)


def get_reserve(campaign):
    if campaign == "aug2020":  # https://st.yandex-team.ru/PLN-513
        reserve_list = [
            ("MAN", 20000, True, "2021-09-30"),  # from reserves https://st.yandex-team.ru/PLN-504
            ("MAN", 20000, True, "2021-12-31"),  # from reserves https://st.yandex-team.ru/PLN-504
            ("SAS", 50000, True, "2021-06-30"),  # from reserves https://st.yandex-team.ru/PLN-504

            ("SAS", 10000, True, "2021-09-30"),  # from AMD at basesearch https://st.yandex-team.ru/PLN-522
            ("SAS", 30000, True, "2021-12-31"),  # from AMD at basesearch https://st.yandex-team.ru/PLN-522

            ("SAS", 22000, True, "2021-09-30"),  # https://st.yandex-team.ru/PLN-602
            ("MAN", 12500, True, "2021-09-30"),  # https://st.yandex-team.ru/PLN-602
        ]
    elif campaign == "feb2021":  # https://st.yandex-team.ru/PLN-784
        reserve_list = [
            ("SAS", 33000, True, "2021-09-30"),  # https://st.yandex-team.ru/PLN-784#60d4c971aeeb795bf86af240

            ("VLA", 8946, True, "2021-09-30"),  # https://st.yandex-team.ru/PLN-784#60d4c90176bc480792917a46
            ("MAN", 630, True, "2021-09-30"),  # https://st.yandex-team.ru/PLN-784#60d4c90176bc480792917a46
            ("SAS", 4662, True, "2021-09-30"),  # https://st.yandex-team.ru/PLN-784#60d4c90176bc480792917a46
        ]
    elif campaign == "aug2021":
        reserve_list = []
    else:
        assert False

    reserve = defaultdict(resources_counter.ResourcesCounter)
    for dc, cpu, full_hosts, date in reserve_list:
        if full_hosts:
            resource = resources_counter.cpu_to_full_hosts(cloud="yp", dc=dc, cpu=cpu)
        else:
            resource = resources_counter.empty_resources()
            resource["dc"] = dc
            resource["cloud"] = "yp"
            resource["cpu"] = cpu

        reserve[date].add_resource(resource)

    return reserve


def upload_reserve(campaign):
    reserve = get_reserve(campaign)

    cloud = "YP"
    segment = "Default"
    add_command_raw = "./dispenser_orders.py --campaign {campaign} --action Create --provider {cloud} --resource \'{resource}\' --order-date {date} --segment-1 {dc}  --segment-2 {segment} --unit={unit} --amount {amount} --confirm Y"

    for date, resource in sorted(reserve.iteritems()):
        print date
        print resource.format()
        print
    print

    names_map = {
        "mem": "ram",
        "ssd_io": "io ssd",
        "hdd_io": "io hdd",
    }

    units_map = {
        "cpu": "Cores",
        "mem": "GiB",
        "ssd": "TiB",
        "hdd": "TiB",
        "ssd_io": "MB/s",
        "hdd_io": "MB/s",
    }

    for date, resource in sorted(reserve.iteritems()):
        for location_key, data in sorted(resource.resources.iteritems()):
            dc = data["dc"]
            for key, val in data.iteritems():
                if key not in resources_counter.RESOURCES_NAMES or not val:
                    continue

                name = names_map.get(key, key).upper()
                unit = units_map[key]

                command = add_command_raw.format(
                    campaign=campaign,
                    cloud=cloud,
                    date=date,
                    segment=segment,
                    dc=dc.upper(),
                    resource=name,
                    unit=unit,
                    amount=val,
                )
                print command
            print

    remove_command = "./dispenser_orders.py --campaign {campaign} --action Remove --provider {cloud} --order-date {date} --confirm Y"
    for date in sorted(reserve.keys()):
        print remove_command.format(campaign=campaign, cloud=cloud, date=date)
    print

    command = "./dispenser_orders.py --campaign {campaign} --action List --provider {cloud} --order-date {date}"
    for date in sorted(reserve.keys()):
        print command.format(campaign=campaign, cloud=cloud, date=date)
    print

    command = "./dispenser_orders.py --campaign {campaign} --action Usage --provider {cloud} --order-date {date}"
    for date in sorted(reserve.keys()):
        print command.format(campaign=campaign, cloud=cloud, date=date)

    print


def evaluate_bot_order(total_resources, campaign, hwr=False):
    def _print_resources(resources):
        clouds = sorted(set([el["cloud"] for el in resources.resources.itervalues()]))

        sys.stdout.write(resources.format(total=True))
        sys.stdout.write("\n\n")

        for cloud in clouds:
            sys.stdout.write("%s check:\n" % cloud)
            for dc in resources_counter.DC_LIST:
                dc_resources = resources.total(filter={"dc": dc, "cloud": cloud})
                if not dc_resources:
                    continue
                dc_hosts = int(resources_counter.resources_to_hosts(dc_resources))
                sys.stdout.write(
                    "%s %s hosts, dominant resources: %s\n" % (
                        dc,
                        dc_hosts,
                        _format_dominant_order(dc_resources)
                    ))
            sys.stdout.write("\n")

        sys.stdout.write("total hosts: %s\n" % resources_counter.resources_to_hosts(resources.total()))
        sys.stdout.write("\n")

    def _change_cloud_to_yp(resources):
        res = resources_counter.ResourcesCounter()
        for key, val in resources.resources.iteritems():
            val = copy.deepcopy(val)
            val["cloud"] = "yp"

            res.add_resource(val)

        return res

    print "total order"
    _print_resources(total_resources)

    total_to_yp = _change_cloud_to_yp(total_resources)

    reserve = resources_counter.ResourcesCounter()
    for date, resources in get_reserve(campaign=campaign).iteritems():
        reserve += resources

    print "reserve"
    _print_resources(reserve)

    to_order = total_to_yp - reserve

    print "total merged to yp minus reserve"
    _print_resources(to_order)

    if hwr:
        _write_order_for_hwr(to_order)


def _write_order_for_hwr(resources):
    map = {
        "cpu": "min_sum_cpu_threads_after_taxes",
        "mem": "min_sum_ram_size_gb_after_taxes",
        "ssd": "min_sum_nvme_size_tb_after_taxes",
        "hdd": "min_sum_hdd_size_tb_after_taxes",
    }

    total = resources.total()
    for key, val in total.iteritems():
        key = map.get(key)
        if not key:
            continue

        print "%s: %s" % (key, max(val, 0))


def set_dispenser_request_status(request_id, status):
    url = "https://dispenser.yandex-team.ru/common/api/v1/quota-requests/{request}/status/{status}"
    url = url.format(request=request_id, status=status)

    response = requests.put(url, verify=False,
                            headers={'Authorization': 'OAuth {}'.format(get_dispenser_token())})

    if response.status_code != 200:
        raise Exception("Dispenser error: %s\n\n" % response.json())


def _load_tickets_to_request_data(reload=False):
    tickets_to_request_data = {}
    for order in load_orders(reload=reload)["result"]:
        tickets_to_request_data[order["trackerIssueKey"]] = order

    return tickets_to_request_data


def approve_dispenser_tickets(tickets, reload=False):
    tickets_to_request_data = _load_tickets_to_request_data(reload=reload)

    target_status = "CONFIRMED"

    for ticket in sorted(set(tickets)):
        request = tickets_to_request_data[ticket]
        if request["status"] == target_status:
            continue

        request_id = request["id"]
        sys.stdout.write("approve https://st.yandex-team.ru/%s %s\n" % (ticket, request_id))
        set_dispenser_request_status(request_id, status=target_status)


def do_approve(args, reload=False):
    if args:
        data = " ".join(args)
    else:
        data = sys.stdin.read()

    tickets = sorted(set(data.replace(",", " ").split()))

    approve_dispenser_tickets(tickets, reload=reload)


def bot_check():
    url = "https://dispenser-test.yandex-team.ru/common/api//v1/bot/pre-orders/from-requests"

    data = json.loads(
        '[{"calculations": "","type": "RESOURCE_PREORDER","projectKey": "yandex","serviceKey": "rtmr","orderId": "40","changes": [{"segmentKeys": [],"resourceKey": "mirror_rps","amount": {"value": 12,"unit": "COUNT"}}, {"segmentKeys": [],"resourceKey": "mirror_storage","amount": {"value": 12,"unit": "GIBIBYTE"}}, {"segmentKeys": [],"resourceKey": "cpu_processing","amount": {"value": 12,"unit": "CORES"}}, {"segmentKeys": [],"resourceKey": "storage_processing","amount": {"value": 12,"unit": "GIBIBYTE"}}, {"segmentKeys": [],"resourceKey": "io_processing","amount": {"value": 12,"unit": "MIBPS"}}]}]')

    resp = requests.post(url,
                         verify=False,
                         headers={'Authorization': 'OAuth {}'.format(get_dispenser_token())},
                         json=data
                         )

    print resp
    print tools.dump_json(resp.json())


def check_orders(campains):
    if not campains:
        campains = ["aug2019"]

    data = load_orders(reload=True)

    runtime_clouds = [
        "qloud",
        "gencfg",
        "yp"
    ]

    logins_to_check = [
        "sereglond",
        "glebskvortsov"
    ]

    res = []

    for order in data["result"]:
        if order["status"] in ["CANCELLED", "REJECTED"]:
            continue

        order_task = order["trackerIssueKey"]

        campaign = order["campaign"]["key"]
        if campaign not in campains:
            continue

        if order["service"]["key"] not in runtime_clouds:
            continue

        task = startrek.get_issue(order_task)

        good_task = False

        for comment in task.comments:
            if not comment.createdBy or comment.createdBy.id not in logins_to_check:
                continue

            text = comment.text.lower()

            if "skip_resources" in text:
                continue

            if u"осталось" in text or "loc:" in text:
                good_task = True
                break

        if good_task:
            res.append(order_task)

    print "\n".join(res)


def print_remainders(orders):
    orders = " ".join(orders).replace(",", " ").split()

    orders_remaining, orders_ready = get_dispenser_orders_resources_data(config=resources_counter.load_config(),
                                                                         reload=True,
                                                                         all_campaigns=True,
                                                                         runtime_only=True
                                                                         )

    replacements = [
        ("-gpu:", "-gpu_q:"),
        ("-hdd_io:", "-io_hdd:"),
        ("-ssd_io:", "-io_ssd:"),
    ]

    def _from_yp_to_dispenser(data):
        for a, b in replacements:
            data = data.replace(a, b)

        return data

    for task in orders:
        task = task.split("/")[-1]

        resources_remaining = orders_remaining.get(task)
        resources_ready = orders_ready.get(task)

        sys.stdout.write(
            "%s remaining:\n%s\n\n" % (
                task, _from_yp_to_dispenser(
                    resources_remaining.format(abc_format=True)) if resources_remaining else "no resources"))
        sys.stdout.write(
            "%s ready:\n%s\n\n" % (
                task,
                _from_yp_to_dispenser(resources_ready.format(abc_format=True)) if resources_ready else "no resources"))


def _close_order(ticket, order_id, comment=None):
    url = 'https://dispenser.yandex-team.ru/common/api/v1/resource-preorder/quotaStateOptional'

    headers = {}
    headers["Authorization"] = "OAuth {}".format(get_dispenser_token())

    data = {}
    request_id_block = {}
    data["updates"] = []
    data["updateFor"] = "BOTH"
    request_id_block['requestId'] = order_id
    if comment:
        request_id_block['comment'] = comment
    data["updates"].append(request_id_block)

    requests.patch(url, headers=headers, json=data, verify=False)
    # TODO add status check


def close_orders(orders, comment=None):
    tickets_to_request_data = _load_tickets_to_request_data(reload=True)

    if orders:
        orders = " ".join(orders).replace(",", " ").split()
    else:
        orders = sys.stdin.read().split()

    for order in orders:
        ticket = order.split("/")[-1]
        request = tickets_to_request_data[ticket]

        status = request["status"]
        if status in ["CANCELLED", "REJECTED", "COMPLETED"]:
            sys.stdout.write("%s is already %s\n" % (ticket, status))
            continue

        order_id = request["id"]

        sys.stdout.write("closing %s\n" % ticket)
        _close_order(ticket, order_id, comment)


def test():
    return


def main():
    from optparse import OptionParser

    parser = OptionParser()

    parser.add_option("--summary", action="store_true")
    parser.add_option("--hwr", action="store_true")

    parser.add_option("--upload-reserve", action="store_true")
    parser.add_option("--campaign")

    parser.add_option("--config", default="utils/hardware/dispenser.json")

    parser.add_option("--approve", action="store_true")

    parser.add_option("--bot-check", action="store_true")

    parser.add_option("--check-orders", action="store_true")

    parser.add_option("--remainders", action="store_true")

    parser.add_option("--close", action="store_true")
    parser.add_option("--comment", action="store_true")

    parser.add_option("--test", action="store_true")
    parser.add_option("--debug", action="store_true")

    (options, args) = parser.parse_args()

    if options.summary:
        create_summmary(
            config_path=options.config,
            debug=options.debug,
            hwr=options.hwr
        )
    elif options.upload_reserve:
        upload_reserve(campaign=options.campaign)
    elif options.approve:
        do_approve(args, reload=options.reload)
    elif options.bot_check:
        bot_check()
    elif options.check_orders:
        check_orders(args)
    elif options.remainders:
        print_remainders(args)
    elif options.close:
        close_orders(orders=args, comment=options.comment)
    elif options.test:
        test()


if __name__ == "__main__":
    main()
