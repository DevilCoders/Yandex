"""Functions for calculation of effective freq"""

import re
import time

import aux_utils
import aux_msr
import aux_arch
import cpu_topology


def get_effective_freq_aperf(period = 1):
    """Calculate effective freq for Intel/AMD processors (using msr)"""
    # TODO replace with cpuinfo register

    topology = cpu_topology.get_cpu_topology()

    # max_freq = int(open('/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq').read())
    if aux_arch.get_cpu_vendor() == aux_arch.EVendor.INTEL:
        BASE_FREQ = (( aux_msr.read_msr(aux_msr.INTEL_PLATFORM_INFO) >> 8 ) & 255 ) * 10 ** 2
        APERF = aux_msr.INTEL_APERF
        MPERF = aux_msr.INTEL_MPERF
    elif aux_arch.get_cpu_vendor() == aux_arch.EVendor.AMD:
        BASE_FREQ = int(open('/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq').read()) / 1000  # FIXME: do not know if it correct
        APERF = aux_msr.AMD_APERF
        MPERF = aux_msr.AMD_MPERF

    counters = {}

    for core in topology.cores:
        thread = core.threads[0]
        counters[thread.id] = ( aux_msr.read_msr(APERF, cpu = thread.id), aux_msr.read_msr(MPERF, cpu = thread.id) )

    start = time.time()
    time.sleep(period)
    delta = time.time() - start

    freq = []
    for core in topology.cores:
        thead = core.threads[0]
        delta_aperf = aux_msr.read_msr(APERF, cpu = thread.id) - counters[thread.id][0]
        delta_mperf = aux_msr.read_msr(MPERF, cpu = thread.id) - counters[thread.id][1]
        freq.append(int((float(delta_aperf) / delta_mperf) * BASE_FREQ))

    return sum(freq) / len(freq)


def get_effective_freq_cpuinfo(period = 1):
    """Calculate effective freq from /proc/cpuinfo (IS BROKEN IN MANY COMBINATIONS OF KERNEL/CPU_MODEL)"""
    with open('/proc/cpuinfo') as f:
        data = f.read()
        freq_values = [float(x) for x in re.findall('cpu MHz\s+:\s+(.*)', data)]
        return sum(freq_values) / len(freq_values)


def get_effective_freq_dummy(period = 1):
    """NO EFFECTIVE FREQ (do not know how to detect effective freq)"""
    return 0.0


def gen_effective_freq_func():
    """Choose function to calculate effective freq"""
    # check for intel-specific way
    if aux_arch.get_cpu_vendor() in (aux_arch.EVendor.INTEL, aux_arch.EVendor.AMD):
        have_msr_status, have_msr_warn = aux_msr.have_msr()
        if not have_msr_status:
            print aux_utils.red_text(have_msr_warn)
        else:
            return get_effective_freq_aperf

    # check for generic way using cpuinfo
    with open('/proc/cpuinfo') as f:
        data = f.read()
        if len(re.findall('cpu MHz\s+:\s+(.*)', data)) == aux_arch.get_cpu_count():
            return get_effective_freq_cpuinfo

    return get_effective_freq_dummy
