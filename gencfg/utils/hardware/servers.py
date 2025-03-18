#!/usr/bin/env python
# coding=utf-8

import argparse
import csv
import json
import requests
import warnings
import os
import sys

from collections import defaultdict

import abc_api


def get_info_from_bot_api(host_name):
    hardware_dump_api_method = 'https://bot.yandex-team.ru/api/consistof.php'

    dump_host_api = '{0}?inv={1}'.format(hardware_dump_api_method, host_name)

    response = None
    with warnings.catch_warnings():
        warnings.simplefilter('ignore')
        warnings.warn('deprecated', requests.packages.urllib3.exceptions.InsecurePlatformWarning)
        try:
            response = requests.get(dump_host_api)
        except Exception:
            pass

    return response.text.encode('utf-8') if response else None


def _init_csv(delimiter=";"):
    dialect_name = {
        ";": "my_semic",
        ",": "my_comma",
        "\t": "my_tab",
    }[delimiter]

    dialect = csv.excel()
    dialect.delimiter = delimiter

    csv.register_dialect(dialect_name, dialect)

    return dialect_name


def _load_table_from_string(data, names_dict={}, delimiter=";"):
    lines = list(csv.reader(data.split("\n"), dialect=_init_csv(delimiter=delimiter)))
    header = lines[0]
    header_len = len(header)

    res = []

    for line in lines[1:]:
        temp = {}
        for num, val in enumerate(line):
            if num >= header_len:
                continue

            name = header[num]
            name = names_dict.get(name, name)

            temp[name] = val

        res.append(temp)

    return res


def dc_reserves():
    abc_api_holder = abc_api.load_abc_api(reload=True)

    url = "https://s3.mds.yandex.net/ya-auto/dc/reports/all-staff-temporary-use.csv"
    data = requests.get(url).text.encode("utf-8")

    stat = defaultdict(lambda: defaultdict(lambda: defaultdict(list)))
    dc_fix = {
        "MANTSALA": "MAN",
        "VLADIMIR": "VLA",
    }

    # login;number;start;serial;partial;status;item_segment4;item_segment3;item_segment2;item_segment1;room_type;loc_segment1;loc_segment2;loc_segment3;loc_segment4;loc_segment5;loc_segment6;parent
    for row in _load_table_from_string(data):
        if row["status"] != "RESERVED":
            continue

        if row["parent"]: # to skipp parts at servers
            continue

        logins = row["employee"].replace(",", " ").split()
        abc_services = []
        for abc_id in row["abc_id"].replace(",", " ").split():
            abc_services.append("abc:" + abc_api_holder.get_service_slug(int(abc_id)))
        if abc_services:
            logins = abc_services
        login = ",".join(sorted(set(logins)))

        dc = row["loc_segment2"]
        dc = dc_fix.get(dc, dc)

        name = []
        for key in "item_segment3 item_segment2 item_segment1".split():
            name.append(row[key])

        name = "/".join(name)
        name = name.replace(" ", "_")

        stat[login][dc][name].append(row["number"])

    if not stat:
        sys.stderr.write("reserve data is empty\n")
        sys.stderr.write("check %s\n" % url)
        exit(-1)

    out_path = "dc-reserves.txt"
    out = file(out_path, "wb")
    full_out_path = "dc-reserves-full.txt"
    full_out = file(full_out_path, "wb")
    for login, login_data in sorted(stat.iteritems()):
        for dc, dc_data in sorted(login_data.iteritems()):
            for name, val in sorted(dc_data.iteritems(), key=lambda x: (-len(x[1]), x[0])):
                out.write("%s\t%s\t%s\t%s\n" % (dc, name, len(val), login))
                full_out.write("%s\t%s\t%s\t%s\t%s\n" % (dc, name, len(val), login, ",".join(sorted(val))))

    print out_path
    print full_out_path


def _sum_power(power_data, invnums):
    total = defaultdict(int)
    missed = 0

    keys = ["platform_pwr_95", "inv_pwr_95_monthly_min", "inv_pwr_95_monthly_max"]

    for inv in invnums:
        data = power_data.get(inv)
        if not data:
            missed += 1
            # print inv
            continue

        for key in keys:
            total[key] += data[key]

    # print "unknown invnums: %s/%s" % (missed, len(invnums))

    return total


def _print_power(power_data, name, invnums):
    power_stat = _sum_power(power_data, invnums)
    sys.stdout.write("%s:\n" % name)

    for key, val in sorted(power_stat.iteritems()):
        sys.stdout.write("\t%s\t%s\n" % (key, int(val / len(invnums))))

    sys.stdout.write("\tservers\t%s\n" % len(invnums))
    sys.stdout.write(
        "\tutilization_max\t%s%%\n" % int(power_stat["inv_pwr_95_monthly_max"] / power_stat["platform_pwr_95"] * 100))
    sys.stdout.write(
        "\tutilization_min\t%s%%\n" % int(power_stat["inv_pwr_95_monthly_min"] / power_stat["platform_pwr_95"] * 100))

    sys.stdout.write("\n")


def power_stat():
    # https://yql.yandex-team.ru/Operations/5cf7d47d36bdbc56fe840222?editor_page=main
    # dump as json
    power_path = "power_stat.json"

    power_data = {}
    for line in file(power_path):
        line_data = json.loads(line)

        power_data[line_data["inv"]] = line_data

    # ./utils/common/dump_hostsdata.py -i invnum -g VLA_YT_RTC > VLA_YT_RTC.txt
    vla_yt = file("/home/sereglond/source/gencfg/VLA_YT_RTC.txt").read().split()
    _print_power(power_data, "vla_yt", vla_yt)

    # ./utils/common/dump_hostsdata.py -i invnum -g ALL_SEARCH -f "lambda x: x.dc=='sas'" > ALL_SEARCH_SAS.txt
    all_search_sas = file("/home/sereglond/source/gencfg/ALL_SEARCH_SAS.txt").read().split()

    # ./utils/common/dump_hostsdata.py -i invnum -g ALL_SEARCH -f "lambda x: x.dc=='sas' and x.queue in 'sas-08 sas-09'.split()" > ALL_SEARCH_SAS_89.txt
    all_search_sas_89 = file("/home/sereglond/source/gencfg/ALL_SEARCH_SAS_89.txt").read().split()

    # ./utils/common/dump_hostsdata.py -i invnum -g SAS_WEB_BASE > SAS_WEB_BASE.txt
    basesearch_inv = file("/home/sereglond/source/gencfg/SAS_WEB_BASE.txt").read().split()

    _print_power(power_data, "rtc all sas", all_search_sas)
    _print_power(power_data, "rtc other sas", sorted(set(all_search_sas).difference(basesearch_inv)))

    _print_power(power_data, "rtc all sas 8-9", all_search_sas_89)
    _print_power(power_data, "rtc other sas 8-9", sorted(set(all_search_sas_89).difference(basesearch_inv)))

    _print_power(power_data, "basesearch all", basesearch_inv)
    _print_power(power_data, "basesearch 8-9", sorted(set(basesearch_inv).intersection(all_search_sas_89)))

    all_sas_rtc_non_base = set(all_search_sas).difference(basesearch_inv)
    file("all_sas_rtc_non_base.txt", "wb").write("\n".join(sorted(all_sas_rtc_non_base)) + "\n")


def get_ffactor(invnum):
    for line in get_info_from_bot_api(invnum).split("\n"):
        line = line.split()
        if not line or not invnum in line:
            continue

        x = line[3].split("/")
        return "/".join(x[-2:])

    return "unknown"


def print_ffactor():
    for line in sys.stdin:
        line = line.split()
        if not line:
            continue

        invnum = line[1]

        print invnum, get_ffactor(invnum)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cut', action="store_true")

    parser.add_argument('--dc-reserves', action="store_true")

    parser.add_argument('--power-stat', action="store_true")

    parser.add_argument('--ffactor', action="store_true")

    options = parser.parse_args()

    if options.dc_reserves:
        dc_reserves()
    elif options.power_stat:
        power_stat()
    elif options.ffactor:
        print_ffactor()


if __name__ == "__main__":
    main()
