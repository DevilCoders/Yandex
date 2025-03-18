"""Functions to set turbo boost correctly"""

import os
import subprocess

import aux_utils
import aux_msr
import aux_arch


def set_turbo_boost_amd_epyc(enable_tb=False):
    """Disable or enable turbo boost for Amd Epyc using msr"""

    # read C0010015 msr register (25th bit enables/disables TB for AMD)
    msr_value = subprocess.check_output(['rdmsr', '0xC0010015'])
    # update register state
    msr_value = int('0x{}'.format(msr_value), 16)
    if enable_tb:
        if (msr_value & (1 << 25)):
            msr_value -= (1 << 25)
    else:
        if not (msr_value & (1 << 25)):
            msr_value += (1 << 25)
    # write modified msr
    subprocess.check_output(['wrmsr', '-a', '0xC0010015', '0x{0:X}'.format(msr_value)])


def set_turbo_toost_intel_pstate(enable_tb=False):
    """Turbo boost for Intel processors using /sys/devices/system/cpu/intel_pstate/"""

    if enable_tb:
        with open('/sys/devices/system/cpu/intel_pstate/no_turbo', 'w') as f:
            f.write('0')
    else:
        with open('/sys/devices/system/cpu/intel_pstate/no_turbo', 'w') as f:
            f.write('1')

    with open('/sys/devices/system/cpu/intel_pstate/max_perf_pct', 'w') as f:
        f.write('100')
    with open('/sys/devices/system/cpu/intel_pstate/min_perf_pct', 'w') as f:
        f.write('100')


def set_turbo_boost_cpufreq(enable_tb=False):
    """Turbo boost using /sys/devices/system/cpu/cpuX/cpufreq/scaling_max_freq"""
    avail_freqs = [int(x) for x in open('/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies').read().split(' ')]
    tbfreq = avail_freqs[0]
    notbfreq = avail_freqs[1]
    ncpu = aux_arch.get_cpu_count()

    for i in range(ncpu):
        if not enable_tb:
            with open('/sys/devices/system/cpu/cpu%s/cpufreq/scaling_min_freq' % i, 'w') as f:
                f.write(notbfreq)
            with open('/sys/devices/system/cpu/cpu%s/cpufreq/scaling_max_freq' % i, 'w') as f:
                f.write(notbfreq)
        else:
            with open('/sys/devices/system/cpu/cpu%s/cpufreq/scaling_max_freq' % i, 'w') as f:
                f.write(tbfreq)
            with open('/sys/devices/system/cpu/cpu%s/cpufreq/scaling_min_freq' % i, 'w') as f:
                f.write(tbfreq)


def set_turbo_boost_dummy(enable_tb=False):
    """NO TURBO BOOST CHANGE (do not know how to switch on/off turbo boost correctly)"""
    pass


def gen_set_turbo_boost():
    """Choose function to set turbo boost"""
    if aux_arch.get_cpu_vendor() == aux_arch.EVendor.INTEL:
        if os.path.exists('/sys/devices/system/cpu/intel_pstate/'):
            return set_turbo_toost_intel_pstate
        elif os.path.exists('/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies'):
            return set_turbo_boost_cpufreq
        else:
            return set_turbo_boost_dummy
    elif (aux_arch.get_cpu_vendor() == aux_arch.EVendor.AMD) and (aux_arch.get_cpu_vendor_arch() == aux_arch.EVendorArch.EPYC):
        have_msr_status, have_msr_warn = aux_msr.have_msr()
        if not have_msr_status:
            print aux_utils.red_text(have_msr_warn)
            return set_turbo_boost_dummy
        if not aux_utils.have_program('wrmsr'):
            print aux_utils.red_text('WARN: Command <wrmsr> not found (try <apt-get install msr-tools>), using dummy turbo boost setter...')
            return set_turbo_boost_dummy
        return set_turbo_boost_amd_epyc
    else:
        return set_turbo_boost_dummy
