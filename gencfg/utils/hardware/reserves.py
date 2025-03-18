#!/usr/bin/env python
# coding=utf-8

import os
import sys

import abc_api
import gencfg_resources
import pickle
import preorders
import resources_counter
import qloud_resources
import st_orders
import tasks
import yp

import urllib3

urllib3.disable_warnings()

from collections import OrderedDict


def compare_reserves(dc_capacity, services_quota, out_path, sum_keys=[]):
    total_quota = resources_counter.ResourcesCounter()
    for quota in services_quota.itervalues():
        resources_counter.sum_resources(quota, sum_keys, total_quota)

    if sum_keys:
        dc_capacity = resources_counter.sum_resources(dc_capacity, sum_keys)

    locations = sorted(set(dc_capacity.resources.keys() + total_quota.resources.keys()))
    table = []

    cores_per_host = 78
    for location in locations:
        quota_data = total_quota.resources.get(location, {})
        dc_data = dc_capacity.resources.get(location, {})

        if not sum([el for el in quota_data.values() if isinstance(el, (int, float))]) and not sum(
            [el for el in dc_data.values() if isinstance(el, (int, float))]):
            continue

        row = OrderedDict()
        for i, location_key in enumerate(resources_counter.LOCATIONS_KEYS):
            row[location_key] = location[i]

        # to set order
        row["hosts"] = int(dc_data.get("cpu", 0) / cores_per_host)
        row["reserved capacity"] = 0
        row["boundary resource"] = ""
        row["required cores"] = 0
        row["required memory"] = 0
        row["required hosts"] = 0

        total_reserved_capacity = 0
        boundary_resource = None
        for resource in resources_counter.RESOURCES_NAMES:
            quota = quota_data.get(resource, 0)
            capacity = dc_data.get(resource, 0)
            resource_reserved_capacity = int(quota * 100 / capacity if capacity else 0)
            if resource_reserved_capacity > total_reserved_capacity:
                total_reserved_capacity = resource_reserved_capacity
                boundary_resource = resource

            row["%s quota" % resource] = quota
            row["%s capacity" % resource] = capacity
            row["%s reserve" % resource] = capacity - quota
            row["%s perc" % resource] = resource_reserved_capacity

        row["reserved capacity"] = total_reserved_capacity
        row["boundary resource"] = boundary_resource
        required_cores = int(max(quota_data.get("cpu", 0) - dc_data.get("cpu", 0), 0))
        required_memory = int(max(quota_data.get("mem", 0) - dc_data.get("mem", 0), 0))
        row["required cores"] = required_cores
        row["required memory"] = required_memory
        row["required hosts"] = max(required_cores / 76, required_memory / 500)

        table.append(row)

    path = os.path.join(resources_counter.create_resources_dumps_dir(), out_path)
    resources_counter.save_csv(table, path)
    print path


def compare_allocation(abc_api_holder, services_quota, services_allocations, out_path):
    services_names = sorted(set(services_quota.keys()).union(set(services_allocations.keys())))

    table = []

    for service_name in services_names:
        service_quota_data = services_quota.get(service_name)
        service_allocations_data = services_allocations.get(service_name)

        locations = set()
        if service_quota_data:
            locations.update(service_quota_data.resources.keys())
        if service_allocations_data:
            locations.update(service_allocations_data.resources.keys())

        for location in sorted(locations):
            quota_data = service_quota_data.resources.get(location, {}) if service_quota_data else {}
            allocations_data = service_allocations_data.resources.get(location, {}) if service_allocations_data else {}

            if not sum([el for el in quota_data.values() if isinstance(el, (int, float))]) and not sum(
                [el for el in allocations_data.values() if isinstance(el, (int, float))]):
                continue

            row = OrderedDict()
            if isinstance(service_name, frozenset):
                for key, val in sorted(dict(service_name).iteritems()):
                    row[key] = val
            else:
                row["ABC service"] = service_name
                row["top ABC service"] = abc_api_holder.get_service_vs_or_top(service_name)

            for i, location_key in enumerate(resources_counter.LOCATIONS_KEYS):
                row[location_key] = location[i]

            for resource in resources_counter.RESOURCES_NAMES:
                quota = quota_data.get(resource, 0)
                allocations = allocations_data.get(resource, 0)

                row["%s allocations" % resource] = allocations
                row["%s quota" % resource] = quota
                row["%s free" % resource] = quota - allocations

            table.append(row)

    path = os.path.join(resources_counter.create_resources_dumps_dir(), out_path)
    resources_counter.save_csv(table, path)
    print path


def compare_all_reserves(clouds="all", reload=False):
    config = resources_counter.load_config()

    clouds = clouds.replace(",", " ").split()
    if "all" in clouds:
        clouds = ["yp", "qloud", "gencfg", "orders", "bot", "credits", "migrations"]

    sum_dc_capacity = resources_counter.ResourcesCounter()
    sum_services_quota = {}
    sum_services_allocations = {}
    sum_credits = {}

    abc_api_holder = abc_api.load_abc_api(reload=reload)
    sys.stderr.write("load_abc_api\n")

    if "yp" in clouds:
        sys.stderr.write("compare_all_reserves yp\n")

        dc_capacity, services_quota, services_allocations = yp.get_yp_resources_data(abc_api_holder, config,
                                                                                     reload=reload)

        sum_dc_capacity += dc_capacity
        sum_services_quota = resources_counter.join_resources_map(sum_services_quota, services_quota)
        sum_services_allocations = resources_counter.join_resources_map(sum_services_allocations, services_allocations)

        compare_reserves(
            dc_capacity=dc_capacity,
            services_quota=services_quota,
            out_path="dc_capacity_yp.csv"
        )

        compare_allocation(
            abc_api_holder=abc_api_holder,
            services_quota=services_quota,
            services_allocations=services_allocations,
            out_path="dc_allocations_yp.csv"
        )

    if "qloud" in clouds:
        sys.stderr.write("compare_all_reserves qloud\n")

        dc_capacity, services_quota, services_allocations, services_quota_native, services_allocations_native = qloud_resources.get_qloud_resources_data(
            abc_api_holder,
            config,
            reload=reload)

        sum_dc_capacity += dc_capacity
        sum_services_quota = resources_counter.join_resources_map(sum_services_quota, services_quota)
        sum_services_allocations = resources_counter.join_resources_map(sum_services_allocations, services_allocations)

        compare_reserves(
            dc_capacity=dc_capacity,
            services_quota=services_quota,
            out_path="dc_capacity_qloud.csv"
        )

        compare_allocation(
            abc_api_holder=abc_api_holder,
            services_quota=services_quota,
            services_allocations=services_allocations,
            out_path="dc_allocations_qloud_abc.csv"
        )

        compare_allocation(
            abc_api_holder=abc_api_holder,
            services_quota=services_quota_native,
            services_allocations=services_allocations_native,
            out_path="dc_allocations_qloud_native.csv"
        )

    if "gencfg" in clouds:
        sys.stderr.write("compare_all_reserves gencfg\n")

        dc_capacity = gencfg_resources.evaluate_gencfg_reserves(config, reload=reload)

        services_quota, groups_resources = gencfg_resources.load_rtc_groups_data()
        sum_services_quota = resources_counter.join_resources_map(sum_services_quota, services_quota)

        # this is not actual quota but real existing groups already placed on hosts
        dc_capacity += sum(services_quota.itervalues(), resources_counter.ResourcesCounter())

        sum_dc_capacity += dc_capacity

        compare_reserves(
            dc_capacity=dc_capacity,
            services_quota=services_quota,
            out_path="dc_capacity_gencfg.csv"
        )

        resources_counter.save_quota_tables(abc_api_holder, groups_resources, "gencfg_groups", split_tables=False,
                                            skip_fields=["cloud", "segment"])

    if "orders" in clouds:
        sys.stderr.write("compare_all_reserves orders\n")

        orders_remainings = st_orders.get_orders_resources_data(config=config, reload=reload)

        sum_services_quota = resources_counter.join_resources_map(sum_services_quota, orders_remainings)
        # TODO remove it or clean out later?

        compare_reserves(
            dc_capacity=resources_counter.ResourcesCounter(),
            services_quota=orders_remainings,
            out_path="dc_capacity_orders.csv"
        )

    if "bot" in clouds:
        sys.stderr.write("compare_all_reserves bot\n")

        dc_capacity = preorders.evaluate_bot_orders(config)
        sum_dc_capacity += dc_capacity

        compare_reserves(
            dc_capacity=dc_capacity,
            services_quota={},
            out_path="dc_capacity_bot.csv"
        )

    if "credits" in clouds:
        sys.stderr.write("compare_all_reserves credits\n")

        dc_capacity, credits_table = tasks.evaluate_credits(config, reload=reload)
        sum_dc_capacity += dc_capacity
        sum_credits = resources_counter.join_resources_map(sum_credits, credits_table)

        compare_reserves(
            dc_capacity=dc_capacity,
            services_quota={},
            out_path="dc_capacity_credits.csv"
        )

    if "migrations" in clouds:
        sys.stderr.write("compare_all_reserves migrations\n")

        dc_capacity = tasks.evaluate_migrations(config, reload=reload)
        sum_dc_capacity += dc_capacity

        compare_reserves(
            dc_capacity=dc_capacity,
            services_quota={},
            out_path="dc_capacity_migrations.csv"
        )

    # cleaning out empty records
    sum_services_quota = dict([(key, val) for (key, val) in sum_services_quota.iteritems() if val])

    with file(os.path.join(resources_counter.create_resources_dumps_dir(), "sum_services_quota.pickle"), "wb") as out:
        pickle.dump(sum_services_quota, out)

    resources_counter.save_services_quota_sum(sum_services_quota, abc_api_holder=abc_api_holder)
    resources_counter.save_quota_tables(abc_api_holder, sum_services_quota, "services_quota")
    resources_counter.save_quota_tables(abc_api_holder, sum_services_allocations, "services_allocations")

    resources_counter.save_quota_tables(abc_api_holder, sum_credits, "credits")

    sys.stderr.write("compare_all_reserves compare\n")

    compare_reserves(
        dc_capacity=sum_dc_capacity,
        services_quota=sum_services_quota,
        out_path="dc_capacity_all.csv"
    )

    compare_allocation(
        abc_api_holder=abc_api_holder,
        services_quota=sum_services_quota,
        services_allocations=sum_services_allocations,
        out_path="dc_allocations_all.csv"
    )

    compare_reserves(
        dc_capacity=sum_dc_capacity,
        services_quota=sum_services_quota,
        out_path="dc_capacity_all_sum.csv",
        sum_keys=["cloud", "segment"]
    )


def test():
    return


def main():
    from optparse import OptionParser

    parser = OptionParser()

    parser.add_option("--reserves", action="store_true")
    parser.add_option("--clouds", default="all")

    parser.add_option("--reload", action="store_true")

    parser.add_option("--test", action="store_true")

    (options, args) = parser.parse_args()

    if options.reserves:
        compare_all_reserves(clouds=options.clouds, reload=options.reload)
    if options.test:
        test()


if __name__ == "__main__":
    main()
