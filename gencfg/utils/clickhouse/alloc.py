import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time
import gencfg  # NOQA

from collections import defaultdict
from operator import itemgetter

from gaux.aux_clickhouse import run_query
from core.db import CURDB

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'putilin_graphs')))
from zoom_data import get_ch_formatted_date_from_timestamp, _run_single_value_query
import json


def get_in_statement_list(group_list):
    return "({})".format(", ".join(["'{}'".format(group_name) for group_name in group_list]))


def get_quota_mismatch_groups(group_list, memusage=True):
    now = time.time()
    seconds_in_week = 7 * 24 * 60 * 60
    usage_query = """ SELECT ts, group, sum({field})
                      FROM instanceusage_aggregated_2m
                      WHERE group in {group_list}
                            AND eventDate >= '{ed1}' AND eventDate <= '{ed2}'
                            {extra_filter_conditions}
                      GROUP BY ts, group
                  """.format(
        field="instance_memusage" if memusage else "instance_cpuusage_power_units",
        group_list=get_in_statement_list(group_list),
        ed1=get_ch_formatted_date_from_timestamp(now - seconds_in_week),
        ed2=get_ch_formatted_date_from_timestamp(now),
        extra_filter_conditions="AND instance_cpuusage <= 1.0" if not memusage else ""
    )

    usage_data = [(ts, group, float(s)) for ts, group, s in run_query(usage_query)]

    allocated_query = """ SELECT ts, group, {field}
                          FROM group_allocated_resources
                          WHERE eventDate >= '{ed1}' AND eventDate <= '{ed2}'
                      """.format(
        field="memory_guarantee" if memusage else "cpu_guarantee",
        group_list=get_in_statement_list(group_list),
        ed1=get_ch_formatted_date_from_timestamp(now - 2 * seconds_in_week),
        ed2=get_ch_formatted_date_from_timestamp(now)
    )

    events = defaultdict(list)
    allocated_data = [(ts, group, float(s)) for ts, group, s in run_query(allocated_query)]

    for ts, group, usage in usage_data:
        events[group].append((ts, ('usage', usage)))

    for ts, group, alloc in allocated_data:
        if group in events:
            events[group].append((ts, ('allocated', alloc)))

    for group, group_events in events.iteritems():
        group_events.sort(key=itemgetter(0))

        total_events = 0
        under_quota_events = 0
        over_quota_events = 0

        assert group_events[0] != 'allocated'
        for event in group_events:
            event_type, event_value = event[1]
            if event_type == 'allocated':
                current_alloc = event_value
            elif event_type == 'usage':
                usage = event_value
                total_events += 1
                if usage < current_alloc * 0.9:
                    under_quota_events += 1
                elif usage > current_alloc * 1.1:
                    over_quota_events += 1
            else:
                print event_type
                assert False

        if float(under_quota_events) / total_events > 0.05:
            print "Under quota", group
        elif float(over_quota_events) / total_events > 0.05:
            print "Over quota", group
        else:
            print "OK group", group


def print_group_usage(group, memusage):
    end_ts = int(time.time())
    start_ts = end_ts - 7 * 24 * 60 * 60

    start_date = get_ch_formatted_date_from_timestamp(start_ts)
    end_date = get_ch_formatted_date_from_timestamp(end_ts)

    table_name = "instanceusage_aggregated_2m"

    usage_query = """
        SELECT max(quant_sum) FROM (
            SELECT quantile(0.97)(sum) as quant_sum
            FROM (SELECT eventDate, sum({field}) as sum
                  FROM {table_name}
                  WHERE eventDate >= '{start_date}' AND eventDate <= '{end_date}' AND group='{group}'
                  GROUP BY eventDate, ts)
            GROUP BY eventDate
        )
    """.format(
        table_name=table_name,
        start_date=start_date,
        end_date=end_date,
        group=group,
        field="instance_memusage" if memusage else "instance_cpuusage_power_units",
    )

    max_usage = _run_single_value_query(usage_query)

    if not max_usage:
        return

    max_usage = float(max_usage) if max_usage else None

    alloc_query = """
        SELECT max({field_allocated})
        FROM group_allocated_resources
        WHERE eventDate >= '{start_date}' AND eventDate <= '{end_date}' AND group='{group}'
    """.format(
        field_allocated="memory_guarantee" if memusage else "cpu_guarantee",
        start_date=start_date,
        end_date=end_date,
        group=group
    )

    max_alloc = float(_run_single_value_query(alloc_query))

    print "JSON", json.dumps({"group": group, "max_usage": max_usage, "max_alloc": max_alloc, "memusage": memusage})


def main():
    all_dynamic = [g.card.name for g in CURDB.groups.get_group('ALL_DYNAMIC').card.slaves]

    for group in all_dynamic:
        for memusage in [False, True]:
            print_group_usage(group, memusage)

    # print "===== CPU ====="
    # get_quota_mismatch_groups(all_dynamic, memusage=False)

    # print "===== MEM ====="
    # get_quota_mismatch_groups(all_dynamic, memusage=True)


if __name__ == "__main__":
    main()
