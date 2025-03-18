#!/usr/bin/env python
# coding=utf-8

import os
import json
import pickle
import subprocess
import sys

from collections import defaultdict

import abc_api
import resources_counter


def arcadia_path():
    return os.path.realpath(os.path.join(os.path.dirname(sys.argv[0]), "../../../"))


def _yp_installations():
    res = []
    for dc in resources_counter.DC_LIST:
        res.append((dc, dc, "yp"))

    res.append(("man-pre", "sas", "yp-pre"))

    return res


def _load_yp_services_quota_piece(abc_api_holder, command_base, setup, dc, cloud, selector, data_key, out_dict):
    import yt.yson  # pip install -i https://pypi.yandex-team.ru/simple/ yandex-yt

    command = command_base.format(
        arcadia=arcadia_path(),
        setup=setup,
        selector=selector
    )

    print command

    # data = json.loads(subprocess.check_output(command, shell=True))
    data = subprocess.check_output(command, shell=True)
    data = yt.yson.loads(data)

    for el in data:
        abc_service, data = el

        if not abc_service.startswith("abc:service:"):  # filtering out system quotas
            continue

        abc_id = int(abc_service.split(":")[-1])
        abc_service = abc_api_holder.get_service_slug(abc_id)
        if not abc_service:
            abc_service = "abc_unknown.%s" % abc_id

        quota = resources_counter.ResourcesCounter()
        for segment, segment_settings in data.get(data_key, {}).get("per_segment", {}).iteritems():
            quota.add_resources_from_yp_json(dc, segment, segment_settings, cloud=cloud)

        out_dict[abc_service] += quota


def load_yp_services_quota(abc_api_holder, reload=False):
    res_pickle_path = os.path.join(resources_counter.create_resources_dumps_dir(), "all_yp_services.pickle")
    if not reload and os.path.exists(res_pickle_path):
        with file(res_pickle_path) as f:
            return pickle.load(f)

    print "load_yp_services_quota"

    services_quota = defaultdict(resources_counter.ResourcesCounter)
    services_allocations = defaultdict(resources_counter.ResourcesCounter)

    for setup, dc, cloud in _yp_installations():
        command_base = "YP_ADDRESS={setup} {arcadia}/ya tool yp select account --selector /meta/id --selector {selector} --no-tabular"

        # 2 calls to get over protobuf message size limit
        _load_yp_services_quota_piece(
            abc_api_holder=abc_api_holder,
            command_base=command_base,
            setup=setup,
            dc=dc,
            cloud=cloud,
            selector="/spec",
            data_key="resource_limits",
            out_dict=services_quota,
        )

        _load_yp_services_quota_piece(
            abc_api_holder=abc_api_holder,
            command_base=command_base,
            setup=setup,
            dc=dc,
            cloud=cloud,
            selector="/status",
            data_key="resource_usage",
            out_dict=services_allocations,
        )

    with file(res_pickle_path, "wb") as f:
        pickle.dump((services_quota, services_allocations), f)

    return services_quota, services_allocations


def get_yp_segments(setup):
    command = "YP_ADDRESS={setup} {arcadia}/ya tool yp select node_segment --selector /meta/id --format json --no-tabular".format(
        arcadia=arcadia_path(),
        setup=setup
    )

    data = json.loads(subprocess.check_output(command, shell=True))
    # [[u'base-search'], [u'base-search-cohabitation'], [u'default'], [u'dev'], [u'gencfg-coexistence-test'], [u'yt_hahn']]
    res = []
    for el in data:
        res.extend(el)

    return res


def load_yp_dc_capacity(segments_to_skip=None, reload=False):
    res_pickle_path = os.path.join(resources_counter.create_resources_dumps_dir(), "all_yp_capacity.pickle")
    if not reload and os.path.exists(res_pickle_path):
        with file(res_pickle_path) as f:
            res = pickle.load(f)
            old_segments_to_skip, saved_result = res
            if segments_to_skip is not None:
                segments_to_skip = sorted(segments_to_skip)

            if old_segments_to_skip == segments_to_skip:
                return saved_result

    print "load_yp_dc_capacity"
    reserves = resources_counter.ResourcesCounter()

    tool_type = "new"
    if tool_type == "old":
        # /status/schedulable_resources can be used to get available resources excluding offline hosts
        # /status/total_resources will get all resources including decommissioned hosts
        command_base = "YP_ADDRESS={setup} {arcadia}/ya tool yp get node_segment {segment} --selector /status/schedulable_resources --format json"
    else:
        # https://st.yandex-team.ru/PLN-804
        # per host evaluation, slower, but gives correct information
        # TODO switch to ya tool when it's ready
        command_base = "{arcadia}/infra/yp_util/bin/yp-util nodes-resources --address {setup} --segment {segment} --node-filter '[/status/hfsm/state] != \"down\" or [/labels/present_in_oops] = %true' --format json --no-pretty-units --node-status ''"

    for setup, dc, cloud in _yp_installations():
        for segment in get_yp_segments(setup):
            if resources_counter.is_bad_segment(segment, segments_to_skip):
                print "skipping segment: %s, %s, %s, %s" % (setup, dc, cloud, segment)
                continue

            command = command_base.format(
                arcadia=arcadia_path(),
                setup=setup,
                segment=segment
            )

            print command

            data = subprocess.check_output(command, shell=True)
            if tool_type == "old":
                for el in json.loads(data):
                    reserves.add_resources_from_yp_json(dc, segment, el, cloud=cloud)
            elif data.strip() == "Nothing found with these parameters":
                    print "no data for segment: %s, %s, %s, %s" % (setup, dc, cloud, segment)
            else:
                for el in json.loads(data):
                    if el["host"] == "":
                        reserves.add_resources_from_yp_json(dc, segment, el, cloud=cloud, json_type="yp_util")

    with file(res_pickle_path, "wb") as f:
        pickle.dump((segments_to_skip, reserves), f)

    return reserves


def get_yp_resources_data(abc_api_holder, config, reload=False):
    segments_to_skip = config.get("yp", {}).get("segments_to_skip", [])

    dc_capacity = load_yp_dc_capacity(segments_to_skip=segments_to_skip, reload=reload)
    services_quota, services_allocations = load_yp_services_quota(abc_api_holder, reload=reload)

    resources_counter.filter_out_segments(dc_capacity, segments_to_skip)
    resources_counter.filter_out_segments(services_quota, segments_to_skip)
    resources_counter.filter_out_segments(services_allocations, segments_to_skip)

    return dc_capacity, services_quota, services_allocations


def print_yp_quota_sum():
    counter = resources_counter.ResourcesCounter()
    for el in sys.stdin.read().split():
        counter.add_resources_from_abc_line(el)

    sys.stdout.write("Sum:\n%s\n" % counter.format())


def evaluate_resources():
    data = sys.stdin.read().replace(",", " ").split()

    groups = []
    services = []
    for el in data:
        if "gencfg" in el:
            el = el.split("/")[-1]
            groups.append(el)
        elif "nanny" in el:
            el = el.split("/")[-1]
            services.append(el)
        else:
            groups.append(el)

    command = "/home/sereglond/source/arcadia/ya tool yp-util account estimate"
    if groups:
        command += " --gencfg_groups %s" % (",".join(sorted(set(groups))))
    if services:
        command += " --nanny_services %s" % (",".join(sorted(set(services))))

    if not groups and not services:
        print "No groups or services found"
        return

    print command

    os.system(command)


def yp_abc(services):
    command_base = "/home/sereglond/source/arcadia/ya tool yp-util account explain --output-limit 10"
    for service in services:
        for cluster in "sas man vla myt iva".split():
            command = "%s abc:service:%s --cluster %s" % (command_base, service, cluster)

            print command

            os.system(command)


def main():
    from optparse import OptionParser

    parser = OptionParser()

    parser.add_option("--sum-quota", action="store_true")
    parser.add_option("--evaluate", action="store_true")
    parser.add_option("--abc", action="store_true")

    (options, args) = parser.parse_args()

    if options.sum_quota:
        print_yp_quota_sum()
    if options.evaluate:
        evaluate_resources()
    if options.abc:
        yp_abc(args)


if __name__ == "__main__":
    main()
