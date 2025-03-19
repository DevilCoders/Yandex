#!/usr/bin/env python

def _describe_cpus(src):
    cpus_list = []
    with open(src, "r") as cpuinfo:
        data = cpuinfo.read()
        for cpu_descr in data.split("\n\n"):
            cpu_descr_dict = {}
            if cpu_descr:
                for cpu_descr_line in cpu_descr.split("\n"):
                    key, val = cpu_descr_line.split(":")
                    if val.lstrip().isdigit():
                        cpu_descr_dict[key.strip()] = int(val.lstrip())
                    else:
                        cpu_descr_dict[key.strip()] = val.lstrip()
                cpus_list.append(cpu_descr_dict)
    return cpus_list

def check_cpu(cpu_description):
    message = {}
    lowfreq_cpu_cores = {}
    bad_governor_mode_cores = {}
    for core in cpu_description:
        if float(core['cpu MHz']) < 1200:
            lowfreq_cpu_cores[core['processor']] = core['cpu MHz']
        with open("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_governor".format(core['processor']), "r") as governor_mode:
            if governor_mode.read().strip() != "performance":
                bad_governor_mode_cores[core['processor']] = governor_mode.read().strip()

    if lowfreq_cpu_cores:
        message['low freq cores'] = "cpu freq is low on cores {}".format(lowfreq_cpu_cores)
    if bad_governor_mode_cores:
        message['governor mode'] = "cores {} are not in performance mode".format(bad_governor_mode_cores)
    return message


if __name__ == '__main__':
    CPUINFO = "/proc/cpuinfo"
    CPU_DESCRIPTION = _describe_cpus(CPUINFO)
    RESULT_CHECK = check_cpu(CPU_DESCRIPTION)
    if RESULT_CHECK:
        print("PASSIVE-CHECK:cpu_perf;2;{}".format(RESULT_CHECK))
    else:
        print("PASSIVE-CHECK:cpu_perf;0;{}".format("OK"))
