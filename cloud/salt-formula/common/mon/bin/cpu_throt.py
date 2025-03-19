#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Simple check for cpu throttling

import sys
from os import uname
from os.path import exists, join

SUPPORTED_CPU = {"E5-2650 v2": {"BIOS": 2600000, "TEMP": 84000, "CORES": 32, "SOCKET": 2},
                 "E5-2660": {"BIOS": 2201000, "TEMP": 85000, "CORES": 32, "SOCKET": 2},
                 "E5-2660 v4": {"BIOS": 3200000, "TEMP": 88000, "CORES": 56, "SOCKET": 2},
                 "E5-2667 v4": {"BIOS": 2600000, "TEMP": 88000, "CORES": 32, "SOCKET": 2},
                 "E5-2690 v4": {"BIOS": 3500000, "TEMP": 88000, "CORES": 56, "SOCKET": 2},
                 "E5-2683 v4": {"BIOS": 3000000, "TEMP": 88000, "CORES": 64, "SOCKET": 2},
                 "Gold 6230": {"BIOS": 3900000, "TEMP": 88000, "CORES": 80, "SOCKET": 2},
                 }

collector = {'thermal': {}, 'capping': {}, 'turboboost': {}, 'perf': {}}


def print_status(service, list_checks, status):
    print("PASSIVE-CHECK:{};{};{}".format(str(service), status, list_checks))


def status_collector(signal, message, status):
    if not collector[signal].get(message):
        collector[signal][message] = status


def read_content(file):
    with open(file, "rt") as src:
        return src.read()


def capping_detect(cpu_key, cpu_sys_path):
    bios_limit = cpu_key["BIOS"]
    min_capped_cpus_freq = bios_limit
    capped_cpus = int()
    for cpuid in range(cpu_key["CORES"]):
        bios_path = join(cpu_sys_path, ("cpu" + str(cpuid)), "cpufreq", "bios_limit")
        if exists(bios_path):
            cur_bios_limit = int(read_content(bios_path))
            if cur_bios_limit < bios_limit:
                min_capped_cpus_freq = min(cur_bios_limit, min_capped_cpus_freq)
                capped_cpus += 1
        else:
            status_collector("capping", "bios_limit section not detected for cpuid {}".format(cpuid), 1)
    if capped_cpus:
        status_collector("capping", ("Capping detected on " + str(capped_cpus) + " CPUs, min_freq: " + str(
            min_capped_cpus_freq / 1000000) + " GHz").upper(), 2)


def turboboost_check(cpu_sys_path):
    boost_path = join(cpu_sys_path, "intel_pstate", "no_turbo")
    if exists(boost_path):
        boost_val = int(read_content(boost_path))
        if boost_val != 0:
            status_collector("turboboost", "TurboBoost disabled".upper(), 2)
    max_perf_pct_path = join(cpu_sys_path, "intel_pstate", "max_perf_pct")
    if exists(max_perf_pct_path):
        max_perf_pct_val = int(read_content(max_perf_pct_path))
        if max_perf_pct_val != 100:
            status_collector("perf", "max_perf_pct less then 100".upper(), 2)
    else:
        status_collector("turboboost", "TurboBoost not supported", 1)


def temp_check(cpu_key, virtual_sys_path):
    affected_packs = int()
    temp = cpu_key["TEMP"]
    max_temp = temp
    for pack in range(cpu_key["SOCKET"]):
        temp_path = join(virtual_sys_path, "thermal", "thermal_zone" + str(pack), "temp")
        if exists(temp_path):
            cur_temp = int(read_content(temp_path))
            if cur_temp >= temp:
                affected_packs += 1
                max_temp = max(cur_temp, max_temp)
        else:
            status_collector("thermal", "Thermal section not detected", 1)
    if affected_packs:
        status_collector("thermal",
                         ("Critical thermal status on " + str(affected_packs) + " sockets, max_temp: " + str(
                             max_temp / 1000).upper() + '; good_temp: less then' + str(temp / 1000)).upper(), 2)


def get_status(list_checks):
    state = 0
    is_supported_cpu = False
    cpuinfo = read_content("/proc/cpuinfo")
    root_sys_cpu = "/sys/devices/system/cpu"
    root_sys_virtual = "/sys/devices/virtual"
    for key in SUPPORTED_CPU:
        if key in cpuinfo:
            is_supported_cpu = True
            capping_detect(SUPPORTED_CPU[key], root_sys_cpu)
            turboboost_check(root_sys_cpu)
            temp_check(SUPPORTED_CPU[key], root_sys_virtual)
    if is_supported_cpu:
        for (signal, status) in collector.items():
            if status == {}:
                fin_status = 0
                fin_message = "OK"
            else:
                state = 2
                fin_status = max(status.values())
                fin_message = "; ".join(status.keys())

            name = 'cpu_throt_{}_check'.format(signal)
            list_checks[name]['status'] = fin_status
            list_checks[name]['msg'] = fin_message
        return state, list_checks
    return 1, 'CPU is not supported'


def main():
    list_checks = {
        'cpu_throt_capping_check': {'status': 0, "msg": ""},
        'cpu_throt_thermal_check': {'status': 0, "msg": ""},
        'cpu_throt_turboboost_check': {'status': 0, "msg": ""},
        'cpu_throt_perf_check': {'status': 0, "msg": ""},
    }
    kern_platform, kern_version = (uname()[0], uname()[2])

    if kern_platform != "Linux":
        print_status('cpu_throt', 1, 'Unsupported platform name')
        sys.exit(0)

    status, messages = get_status(list_checks)
    print_status('cpu_throt', messages, status)
    sys.exit(0)


if __name__ == '__main__':
    main()
