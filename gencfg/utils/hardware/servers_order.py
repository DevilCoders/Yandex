#!/usr/bin/env python
# coding=utf-8

import datetime
import math
import os
import sys

from collections import defaultdict

HOST_OPERATION_TICKET = "PLN-271"
DEFAULT_ORDER_ASSIGNEE = "antivabo"


def print_servers_from_requests(requests):
    server_cores_common = 30
    dc_segments_cores = defaultdict(int)

    for request in requests:
        dc_list = request["dc"].split(",")
        if request.get("done"):
            continue

        cores = float(request.get("servers", 0) * server_cores_common + request.get("cores", 0))
        for dc in dc_list:
            dc_segments_cores[(dc, request.get("segment", "unknown"))] += cores / len(dc_list)

    dc_segments_cores = dict([(key, int(math.ceil(val))) for (key, val) in dc_segments_cores.iteritems()])

    dc_cores = defaultdict(int)
    segments_cores = defaultdict(int)
    for key, val in dc_segments_cores.iteritems():
        dc_cores[key[0]] += val
        segments_cores[key[1]] += val

    print_total("dc", dc_segments_cores)
    print_total("segments", segments_cores)
    print_total("cores", dc_cores)

    return dc_segments_cores


def print_total(name, data):
    print name + ":"
    total = 0
    for key, val in sorted(data.iteritems()):
        if isinstance(key, tuple):
            key = ".".join(key)

        if isinstance(val, list):
            val = len(val)

        if val:
            print key, val
            total += val

    print "total", total
    print


def new_wave(order=False, manual_order=False, debug=False, task=None):
    requests = [
        {  # QLOUDRES-1267
            "cores": 55 * 4,
            "dc": "vla",
            "segment": "qloud-ext.common-zen",
            "done": True
        },
        {  # QLOUDRES-1267
            "cores": 200,
            "dc": "man",
            "segment": "qloud-ext.common-zen",
            "done": True
        },

        {  # RX-1337
            "servers": 2 * 2,
            "dc": "man,sas",
            "segment": "qloud-ext.sox",
            "done": True
        },
        {  # RX-1348
            "cores": 150,
            "dc": "vla",
            "segment": "qloud-ext.outsource",
            "done": True
        },
    ]

    current_needs = ""

    segment = None
    for line in current_needs.split("\n"):
        line = line.strip().strip("*").split()
        if not line:
            continue

        if len(line) == 1:
            segment = line[0]
        else:
            assert segment
            requests.append({
                "segment": segment,
                "dc": line[0],
                "cores": int(line[1].strip("!"))
            })

    dc_segments_cores = print_servers_from_requests(requests)

    if order:
        order_servers(dc_segments_cores, manual_order=manual_order, debug=debug, task=task)


def load_manual_order(path):
    res_segment_servers = defaultdict(list)

    segment = None
    temp = []
    for line in file(path):
        sline = line.split()
        if not sline:
            continue

        if len(sline) == 1:
            if segment:
                res_segment_servers[segment].extend(temp)
                temp = []
            segment = sline[0]
            continue

        if not line.endswith("\n"):
            line += "\n"

        temp.append(line)

    if segment:
        res_segment_servers[segment].extend(temp)

    manual_dc_segments = []
    for segment, servers in res_segment_servers.iteritems():
        for dc in set([line.split()[0] for line in servers]):
            manual_dc_segments.append([dc, segment])

    return res_segment_servers, manual_dc_segments


def _get_transaction_servers(dc_servers, req_cores):
    res_servers = []
    res_cores = 0
    for server in dc_servers:
        res_servers.append(server)

        host_quota = eval_hosts_to_quota(server, mode="yp")
        assert len(host_quota) == 1
        res_cores += host_quota.values()[0]["cpu"]

        if res_cores >= req_cores:
            break

    return res_cores, res_servers, dc_servers[len(res_servers):]


def order_servers(dc_segments_cores, manual_order=False, leftovers_segment=None, debug=False, task=None,
                  order_type="qloud-reserve"):
    input_servers = defaultdict(list)
    for line in sys.stdin:
        dc = line.split()[0]
        input_servers[dc].append(line)

    print_total("input servers", input_servers)

    res_segment_servers = defaultdict(list)
    manual_dc_segments = []

    if manual_order:
        assert False  # need tweaks to work with dc_segments_cores instead of servers

        res_segment_servers, manual_dc_segments = load_manual_order(manual_order)
        for segment, servers in res_segment_servers.iteritems():
            for dc in input_servers.keys():
                input_servers[dc] = sorted(set(input_servers[dc]).difference(servers))

        print "manual order segments:", sorted(manual_dc_segments)
        print_total("manual order", res_segment_servers)

    for (dc, segment), req_cores in sorted(dc_segments_cores.iteritems()):
        if [dc, segment] in manual_dc_segments:
            sys.stdout.write("Skipping manual %s.%s\n" % (dc, segment))
            continue

        dc_servers = input_servers.get(dc, [])
        transaction_cores, transaction_servers, leftover_servers = _get_transaction_servers(dc_servers, req_cores)

        if transaction_cores < req_cores:
            raise Exception("Not enough cores at %s: %s vs %s\n" % (dc, transaction_cores, req_cores))

        res_segment_servers[segment].extend(transaction_servers)
        input_servers[dc] = leftover_servers

    if manual_dc_segments:
        print

    all_invnums = set()
    for segment, servers in res_segment_servers.iteritems():
        invnums = [line.split()[1] for line in servers]
        intersection = all_invnums.intersection(invnums)
        if intersection:
            sys.stderr.write("Duplicate invnums at orders: %s\n" % ",".join(sorted(intersection)))
            return

        all_invnums.update(invnums)

    print_total("required order", res_segment_servers)

    print_total("leftovers", input_servers)
    if leftovers_segment:
        print "leftovers are added to %s" % leftovers_segment
        for servers in input_servers.itervalues():
            res_segment_servers[leftovers_segment].extend(servers)

    print_total("total order", res_segment_servers)

    qloud_hosts_transfer(res_segment_servers=res_segment_servers, task=task, debug=debug, order_type="qloud-reserve")


def qloud_hosts_transfer(res_segment_servers, task, debug=False, order_type="qloud-reserve"):
    if order_type == "qloud-reserve":
        import qloud
        if not task:
            raise Exception("--task must be set to transfer hosts from qloud-ext.reserve")

    for segment, servers in sorted(res_segment_servers.iteritems()):
        sys.stdout.write("moving %s hosts to %s via %s\n" % (len(servers), segment, order_type))
        sys.stdout.write("\n".join([el.strip() for el in servers]) + "\n")

        if order_type == "qloud-reserve":
            fqdn_list = [server.split()[2] for server in servers]
            if segment.startswith("qloud-ext."):
                qloud.transfer_hosts(fqdn_list, segment, comment=task, debug=debug)
            elif segment.startswith("qloud."):
                qloud.transfer_hosts(fqdn_list, "qloud-ext.gateway", comment=task,
                                     debug=debug)  # to store servers while waiting for transfer
                make_order(data="".join(servers), segment=segment, debug=debug)
            else:
                assert False
        elif order_type == "sre":
            make_order(data="".join(servers), segment=segment, debug=debug)
        else:
            assert False

        sys.stdout.write("\n")


def eval_hosts_to_quota(data, mode, no_discount=False, full_memory=False, set_memory=None):
    # iva	436121	telephony-qloud-iva1.yandex.ru	E5645	48	0	0
    if mode == "yp-maps":
        mode = "yp"
        full_memory = True

    res = defaultdict(lambda: defaultdict(int))
    for line in data.split("\n"):
        line = line.split()
        if not line:
            continue

        dc = line[0]
        model = line[3]

        # TODO use ncpu from models.yaml
        if model == "E5-2660v4":
            cores = 56
        elif model == "E5-2683v4":
            cores = 64
        elif model == "Gold6130":
            cores = 64
        elif model == "Gold6230":
            cores = 80
        elif model == "Gold6230R":
            cores = 128
        elif model == "Epyc7351":
            cores = 64
        elif model == "Epyc7452":
            cores = 128
        elif model in ["E5-2660", "E5-2650v2", "E5-2667v2", "E5-2667v4"]:
            cores = 32
        elif model in ["E5645", "E5-2667"]:
            cores = 24
        elif model in ["E5-2650v4"]:
            cores = 48
        elif model in ["AMD6274"]:
            cores = 21  # by power
        else:
            raise Exception("unknown process model %s\n" % model)

        memory = int(line[4])
        if set_memory:
            memory = int(set_memory)

        if mode == "qloud":
            memory = memory * 0.95
            cpu_quota = cores * 0.90
        else:
            memory = memory - 7
            cpu_quota = cores - 2

        cpu_quota_orig = cpu_quota
        if not no_discount:
            cpu_quota = min(cpu_quota, memory / 4)

        discount = float(cpu_quota) / cpu_quota_orig

        res[dc]["cpu"] += cpu_quota

        memory_quota = cpu_quota * 4
        if full_memory:
            memory_quota = max(memory_quota, memory)

        res[dc]["memory"] += memory_quota

        ssd = float(line[5])
        hdd = float(line[6])

        res[dc]["hdd"] += (min(2, hdd / 1024) if hdd else 2) * discount
        res[dc]["ssd"] += (min(2, ssd / 1024) if ssd else 0) * discount

    return res


def format_quota(quota):
    res = []

    total = defaultdict(int)
    for dc, dc_data in sorted(quota.iteritems()):
        res.append(
            " ".join(map(str, [dc + ":", dc_data["cpu"], "cores", str(int(dc_data.get("memory", 0))) + "G memory,", str(
                int(dc_data.get("ssd", 0))) + "T ssd,", str(int(dc_data.get("hdd", 0))) + "T hdd"])))

        for key, val in dc_data.iteritems():
            total[key] += val

    res.append(" ".join(map(str, ["total:", total["cpu"], "cores,", str(int(total.get("memory", 0))) + "G memory,",
                                  str(int(total.get("ssd", 0))) + "T ssd,", str(int(total.get("hdd", 0))) + "T hdd"])))

    return "\n".join(res) + "\n"


def print_hosts_to_quota(data, mode="qloud", no_discount=False, full_memory=False, set_memory=None, echo=False):
    if echo:
        sys.stdout.write(data)
        sys.stdout.write("\n")

    sys.stdout.write(format_quota(
        eval_hosts_to_quota(data, mode=mode, no_discount=no_discount, full_memory=full_memory, set_memory=set_memory)))


def make_order(data, task=None, segment=None, silent=False, debug=False, new_format=True):
    import startrek

    if task:
        task = task.split("/")[-1]

    if not segment and task:
        if task.startswith("GENCFG"):
            segment = "rtc"
        elif task.startswith("YPRES"):
            segment = "yp"
        elif segment != "debug":
            raise Exception("segment is not set")

    assert segment

    order_queue = "RUNTIMECLOUD"
    order_tags = "hosts-comp:move"
    order_components = [42199, 51105]  # ["hosts", "rtc_maintenance"]

    order_assignee = DEFAULT_ORDER_ASSIGNEE
    if debug:
        order_queue = "TEST"
        order_assignee = "sereglond"
        if not segment:
            segment = "yp"

    if segment.startswith("yp"):
        mode = "yp"

        order_segment = "yp-iss-[dc]-dev" if "dev" in segment else "yp-iss-[dc]"
    elif segment.startswith("qloud"):
        mode = "qloud"

        order_segment = segment
    elif segment == "rtc":
        mode = "yp"
        order_segment = "rtc-mtn"
    else:
        assert False

    main_task = None
    user = None
    if task:
        main_task = startrek.get_issue(task)
        user = main_task.createdBy.id
        assert user

    invnums_list = [el.split()[1] for el in data.split("\n") if el.strip()]
    hosts_count = len(invnums_list)

    if new_format:
        user_formated = (u"""
Кто сдает хосты:
%s
        """ % user) if user else ""

        new_task_description = u"""
Тип работ:
ввод хостов в wall-e проект

Wall-e проект:
%s
%s
<{Список ID серверов:
%s
}>

<{Детальная информация о хостах
%%%%
%s
%%%%
}>
""".strip() % (order_segment, user_formated, "\n".join(invnums_list), data)

    else:
        new_task_description = "<{Hosts:\n%%\n" + "\n".join(invnums_list) + "\n%%}>\n"

    new_task = startrek.create_issue(
        queue=order_queue,
        summary=u"Ввод %s хостов в wall-e проект %s от %s" % (hosts_count, order_segment, datetime.date.today()),
        description=new_task_description,
        assignee=order_assignee,
        # components=order_components,
        tags=order_tags
    )
    if user:
        new_task.comments.create(text="Пожалуйста, подтверди передачу хостов", summonees=[user])
    sys.stdout.write("new order for %s: %s\n" % (segment, new_task.key))

    if task and not silent:
        formated_quota = format_quota(eval_hosts_to_quota(data, mode=mode))

        main_task_message = "%%\n" \
                            + data \
                            + "\n" + formated_quota \
                            + "%%\n" \
                            + u"Вводим хосты в " + new_task.key
        main_task.comments.create(text=main_task_message)
        sys.stdout.write("order linked to %s\n" % task)

    if debug or silent:
        return

    all_orders_task = startrek.get_issue(HOST_OPERATION_TICKET)
    description = all_orders_task.description
    temp = [new_task.key]
    if task:
        temp.append(task)
    description = description.rstrip() + "\n\n%s\n" % "\n".join(temp)
    all_orders_task.update(description=description)


def main():
    from optparse import OptionParser

    parser = OptionParser()
    parser.add_option('--hosts', action="store_true")
    parser.add_option('--mode', default="yp")

    # print_hosts_to_quota options
    parser.add_option('--no-discount', action="store_true")
    parser.add_option('--full-memory', action="store_true")
    parser.add_option('--set-memory')
    parser.add_option('--echo', action="store_true")

    parser.add_option('--order', action="store_true")
    parser.add_option('--segment')
    parser.add_option('--silent', action="store_true")
    parser.add_option('--task')

    parser.add_option('--wave', action="store_true")
    parser.add_option('--manual-order')  # in addition to wave order
    parser.add_option('--qloud-reserve', action="store_true")  # transfer from qloud-ext.reserve

    parser.add_option('--debug', action="store_true")

    (options, args) = parser.parse_args()

    if options.wave:
        new_wave(order=options.order, manual_order=options.manual_order, debug=options.debug, task=options.task)
    elif options.hosts or options.order:
        mode = options.mode
        data = sys.stdin.read()
        if options.qloud_reserve:
            segment = options.segment
            assert segment.startswith("qloud-ext.")

            servers = [el for el in data.split("\n") if el.strip()]

            res_segment_servers = {segment: servers}

            qloud_hosts_transfer(res_segment_servers, task=options.task, debug=options.debug)
        elif options.order:
            if not options.task and not options.silent:
                sys.stderr.write("--task is not set\n")
                exit(1)

            make_order(data,
                       segment=options.segment,
                       task=options.task,
                       silent=options.silent,
                       debug=options.debug,
                       )
        else:
            print_hosts_to_quota(data, mode=mode,
                                 no_discount=options.no_discount,
                                 full_memory=options.full_memory,
                                 set_memory=options.set_memory,
                                 echo=options.echo)


if __name__ == "__main__":
    main()
