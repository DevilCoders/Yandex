#!/usr/bin/env python
# coding=utf-8

import copy
import math
import re
import os
import pickle
import sys
import tools
import traceback

from collections import OrderedDict

import dispenser
import resources_counter
import startrek

DC_NAMES = "SAS MAN VLA".split()

RESOURCES_TYPES = OrderedDict({
    "cpu": ["CPU"],
    "mem": [r"Память"],
    "gpu": ["GPU"]
})


def parse_form(task):
    res = OrderedDict()
    res["task"] = "https://st.yandex-team.ru/" + task.key.encode("utf-8")
    res["status"] = task.status.key

    key = None
    val = []

    for line in task.description.replace("%%", "%%\n").split("\n"):
        line = line.rstrip()

        if line.startswith("**"):
            key = line.strip("*").strip()
        elif line == "%%":
            if val:
                if not key:
                    sys.stderr.write("form parse error. form without name at %s\n" % task.key)

                if not val:
                    sys.stderr.write("form parse error. no data for field %s:%s\n" % (task.key, key))

                key = key.encode("utf-8")
                if key in res:
                    sys.stderr.write("Duplicate field %s at %s\n" % (key, task.key.encode("utf-8")))

                res[key] = "\n".join(val).encode("utf-8").strip("\n")
                key = None
                val = []
        elif key:
            val.append(line)

    return res


def parse_dc_distribution(task, data):
    dc_list = data.get(r"Датацентры", "")
    dc_list = dc_list.upper().replace(",", " ").split()
    if not dc_list:
        dc_list = DC_NAMES
    else:
        unknown_dc = set(dc_list).difference(DC_NAMES)
        if unknown_dc:
            sys.stderr.write("Unknow dc at %s: %s\n" % (", ".join(sorted(unknown_dc)), task))
            dc_list = DC_NAMES

    dc_count = float(len(dc_list))
    default_dictribution = dict([dc, 1 / dc_count] for dc in dc_list)

    raw_val = data.get(r"Распределение по ДЦ")

    if not raw_val:
        return default_dictribution

    raw_distribution = {}

    try:
        for line in raw_val.replace(",", "\n").replace("%", "").split("\n"):
            line = line.split()
            raw_distribution[line[0].upper()] = float(line[1])
    except:
        sys.stderr.write("dc distribution parse error at %s\n" % task)
        return default_dictribution

    if not raw_distribution:
        sys.stderr.write("no dc distribution data at %s\n" % task)
        return default_dictribution

    d_sum = sum(raw_distribution.values())
    if not d_sum:
        sys.stderr.write("dc distribution sum == 0 at %s\n" % task)
        return default_dictribution

    res = dict([dc, raw_distribution.get(dc, 0) / d_sum] for dc in DC_NAMES)

    return res


def get_dc_distribution(task):
    return parse_dc_distribution(task, parse_form(task))


def _parse_resource(task, data, key):
    raw_val = data.get(key)
    val = raw_val
    try:
        if not val or val == r"Нет ответа":
            return None

        val = val.lower()
        val = val.replace("gb", ' ').replace("g", ' ').strip()
        val = int(val)
        return val

    except:
        traceback.format_exc()
        sys.stderr.write("Failed to parse resources from %s: %s\n" % (task.key, key.decode("utf-8")))

    return None


def parse_resources(task, form_data):
    resources = {}
    for type, keys in RESOURCES_TYPES.iteritems():
        for key in keys:
            val = _parse_resource(task, form_data, key)

            if val:
                resources[type] = val
                break

    return resources


def get_list_of_cloud_tickets(master_ticket):
    description = startrek.get_issue(master_ticket).description

    return re.findall(u'{}-\w+'.format(master_ticket.split("-")[0]), description)


def _evaluate_and_check_remaings(task, left, right):
    left = left.replace(u"ядер", "").replace(u"ядра", "").replace(u"ядро", "").replace(u"cores", "")
    right = right.replace(u"ядер", "").replace(u"ядра", "").replace(u"ядро", "").replace(u"cores", "")

    if not left.strip():
        return 0

    eval_results = None
    try:
        eval_results = float(eval(left))
    except:
        sys.stderr.write(
            u"_evaluate_and_check_remaings eval exception at {}:{}\n{}\n".format(task.key, left,
                                                                                 traceback.format_exc().decode("utf-8"))
        )

    if not right.strip():
        return eval_results if eval_results else 0

    right = float(right.strip().split()[0])

    if eval_results and right and math.fabs(right - eval_results) >= 3:
        sys.stderr.write(
            u"_evaluate_and_check_remaings check mismatch at {}: {} vs {}\n".format(
                task.key,
                eval_results,
                right
            ))

    return right


def get_remaining_resources(task, logins_to_check):
    TOTAL_REMAININGS_PREFIX = u"осталось"
    DC_REMAININGS_PREFIX = u"осталось в"
    assert DC_REMAININGS_PREFIX != TOTAL_REMAININGS_PREFIX

    ORDER_CLOSED_MESSAGE = u"заказ выдан полностью"
    RESOURCE_RECORD_START = u"выдано"
    RESOURCE_RECORD_PREFIX = "loc:"

    remainings_per_dc = {}
    remainings_total = None
    dc_to_substract = None

    close_order = False

    try:
        for comment in task.comments:
            if not comment.createdBy or comment.createdBy.id not in logins_to_check:
                continue

            if close_order:
                break

            if "skip_resources" in comment.text:
                continue

            is_resouces_comment = False

            for line in comment.text.split("\n"):
                line = line.strip().lower().replace("==", "=")

                if line == ORDER_CLOSED_MESSAGE:
                    close_order = True
                    break
                if line == RESOURCE_RECORD_START:
                    is_resouces_comment = True
                    continue
                if line.startswith(TOTAL_REMAININGS_PREFIX) and not line.startswith(DC_REMAININGS_PREFIX):
                    left, _, right = line.replace(TOTAL_REMAININGS_PREFIX, "").partition("=")

                    remainings_total = _evaluate_and_check_remaings(task, left, right)

                    if dc_to_substract is not None:
                        if remainings_total == 0:
                            close_order = True
                            break

                        sys.stderr.write("Remaining and substract mix at %s\n" % task.key)
                        continue

                elif line.startswith(DC_REMAININGS_PREFIX):
                    if dc_to_substract is not None:
                        sys.stderr.write("Remaining and substract mix at %s\n" % task.key)
                        continue

                    left, _, right = line.replace(DC_REMAININGS_PREFIX, "").partition("=")
                    dc = left.split()[0]
                    left = " ".join(left.split()[1:])

                    remainings_per_dc[dc] = _evaluate_and_check_remaings(task, left, right)
                elif is_resouces_comment and RESOURCE_RECORD_PREFIX:
                    resource_records = [el for el in line.split() if el.startswith(RESOURCE_RECORD_PREFIX)]

                    if dc_to_substract is None:
                        dc_to_substract = resources_counter.ResourcesCounter()

                    for el in resource_records:
                        dc_to_substract.add_resources_from_abc_line(el)

        dc_sum = sum(remainings_per_dc.values())
        if dc_sum and remainings_total and math.fabs(remainings_total - dc_sum) >= 3:
            sys.stderr.write("get_resources_remainings dc sum not match with total at %s: %s vs %s\n" % (
                task.key, dc_sum, remainings_total))

        if remainings_total is None and dc_sum:
            remainings_total = dc_sum
    except:
        sys.stderr.write("exception at get_dc_remainings %s\n%s\n\n" % (task.key, traceback.format_exc()))

    if close_order:
        remainings_per_dc = {}
        remainings_total = 0
        dc_to_substract = None

    return remainings_total, remainings_per_dc, dc_to_substract


def get_st_orders_resources_data(config, reload=False):
    res_pickle_path = os.path.join(resources_counter.create_resources_dumps_dir(), "st_orders.pickle")
    if not reload and os.path.exists(res_pickle_path):
        with file(res_pickle_path) as f:
            return pickle.load(f)

    logins_to_check = config["orders"]["reminders_logins"]

    orders = {}

    for head_ticket in config["orders"]["tickets"]:
        for task in sorted(set(get_list_of_cloud_tickets(head_ticket))):
            task = startrek.get_issue(task)
            if task.status.key == "closed":
                continue

            form_data = parse_form(task)
            dc_distribution = parse_dc_distribution(task, form_data)
            full_order_resources = parse_resources(task, form_data)

            cpu_remaining, dc_remaining, dc_to_substract = get_remaining_resources(task,
                                                                                   logins_to_check=logins_to_check)
            if dc_to_substract:
                sys.stderr.write("dc_to_substract at get_st_orders_resources_data at %s\n" % task.key)

            if cpu_remaining == 0:
                continue
            elif cpu_remaining is None:
                cpu_remaining = full_order_resources.get("cpu", 0)

            if dc_remaining:
                lost_dc = set([el.lower() for el in dc_distribution.keys()]).difference(dc_remaining.keys())
                if lost_dc:
                    sys.stderr.write(
                        "DC with unknown remainings at {}: {}\n".format(task.key, ",".join(sorted(lost_dc))))
            else:
                for dc, mult in dc_distribution.iteritems():
                    dc_remaining[dc] = cpu_remaining * mult

            if cpu_remaining > full_order_resources.get("cpu", 0):
                sys.stderr.write("Order remaining is too big {}: {} vs {}\n".format(task.key, cpu_remaining,
                                                                                    full_order_resources["cpu"]))

            order_remaining = resources_counter.ResourcesCounter()
            for dc, dc_cores in dc_remaining.iteritems():
                resource = resources_counter.empty_resources()
                resource["cloud"] = "orders"
                resource["dc"] = dc
                resource["segment"] = head_ticket

                resource["cpu"] = dc_cores

                full_order_cpu = full_order_resources.get("cpu", 0)
                if full_order_cpu:
                    resource["mem"] = full_order_resources.get("mem", 0) * dc_cores / full_order_cpu

                order_remaining.add_resource(resource)

            orders[task.key] = order_remaining

    with file(res_pickle_path, "wb") as f:
        pickle.dump(orders, f)

    return orders


def _remove_order_resource(original_resources, to_remove, task):
    res_resources = copy.deepcopy(original_resources)

    for (cloud, dc, segment), resources in to_remove.resources.iteritems():
        res_dc_resource = res_resources.resources.get(("orders", dc, None))
        if res_dc_resource is None:
            sys.stderr.write("remove_order_resource: missed resource for %s at %s\n" % (dc, task))
            continue

        for resource_name, resource_val in resources.iteritems():
            if resource_name not in resources_counter.RESOURCES_NAMES:
                continue

            diff_val = res_dc_resource.get(resource_name, 0) - resource_val
            if diff_val < 0 and not (resource_name in ["hdd", "ssd"] and diff_val >= -1):
                sys.stderr.write("negative remaining at %s: %s %s %s\n" % (task, dc, resource_name, diff_val))
                diff_val = 0

            res_dc_resource[resource_name] = diff_val

    return res_resources


def get_orders_resources_data(config, reload=False):
    sys.stderr.write("get_orders_resources_data st_orders\n")
    st_orders = get_st_orders_resources_data(config=config, reload=reload)

    sys.stderr.write("get_orders_resources_data dispenser_load\n")
    dispenser_orders_remaining, dispenser_orders_ready = dispenser.get_dispenser_orders_resources_data(config=config, reload=reload)

    assert not set(st_orders.keys()).intersection(dispenser_orders_remaining.keys())

    all_orders = {}
    all_orders.update(st_orders)
    all_orders.update(dispenser_orders_remaining)

    return all_orders
