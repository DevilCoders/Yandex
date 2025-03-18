"""Various functions for calculation of consumption"""

import time

import aux_utils
import aux_arch
import aux_msr
import cpu_topology


def get_package_consumption_rapl_msr(period=1):
    """Consumption of cpu package for Intel/AMD processors usind msr registers"""
    topology = cpu_topology.get_cpu_topology()

    # configure msr registers
    if aux_arch.get_cpu_vendor() == aux_arch.EVendor.INTEL:
        RAPL_PWR_UNIT = aux_msr.INTEL_RAPL_POWER_UNIT
        RAPL_PWR_UNIT_ESU_BITS = aux_msr.INTEL_RAPL_POWER_UNIT_ESU_BITS
        PKG_ENERGY_STATUS = aux_msr.INTEL_PKG_ENERGY_STATUS
    elif aux_arch.get_cpu_vendor() == aux_arch.EVendor.AMD:
        RAPL_PWR_UNIT = aux_msr.AMD_RAPL_PWR_UNIT
        RAPL_PWR_UNIT_ESU_BITS = aux_msr.AMD_RAPL_PWR_UNIT_ESU_BITS
        PKG_ENERGY_STATUS = aux_msr.AMD_PKG_ENERGY_STAT
    else:
        raise Exception('Unsupported vendor <{}>'.format(aux_arch.get_cpu_vendor()))

    rapl_energy_units = 1.0 / (1 << aux_msr.read_msr(RAPL_PWR_UNIT, bits=RAPL_PWR_UNIT_ESU_BITS))

    energy_cpu = {}
    for package in topology.packages:
        energy_cpu[package.id] = aux_msr.read_msr(PKG_ENERGY_STATUS, cpu = package.cores[0].threads[0].id) & 0xFFFFFFFF

    start = time.time()
    time.sleep(period)
    delta = time.time() - start

    consumption = 0.0
    for package in topology.packages:
        new_energy_cpu = aux_msr.read_msr(PKG_ENERGY_STATUS, cpu =  package.cores[0].threads[0].id)
        if new_energy_cpu < energy_cpu[package.id]:
            new_energy_cpu += 0x100000000
        consumption += (new_energy_cpu - energy_cpu[package.id]) * rapl_energy_units / delta

    return consumption


def get_package_consumption_dummy(period=1):
    """NO CONSUMPTION (do not know how to detect correct consumption of package)"""
    return 0.0


def gen_package_consumption_func():
    """Choose function to calculate package consumption"""
    if aux_arch.get_cpu_vendor() in (aux_arch.EVendor.INTEL, aux_arch.EVendor.AMD):
        have_msr_status, have_msr_warn = aux_msr.have_msr()
        if not have_msr_status:
            print aux_utils.red_text(have_msr_warn)
            return get_package_consumption_dummy
        return get_package_consumption_rapl_msr
    else:
        print aux_utils.red_text('WARN: no package consumption method found, using dummy ...')
        return get_package_consumption_dummy


def get_memory_consumption_intel_msr(period=1):
    """Consumption of memory for Intel processors using msr registers"""
    topology = cpu_topology.get_cpu_topology()
    # Energy units for memory on Haswell and later CPUs are hardcoded
    rapl_energy_dram = 1.0 / ( 1 << 0x10 )

    energy_dram = {}
    for package in topology.packages:
        energy_dram[package.id] = aux_msr.read_msr(aux_msr.INTEL_DRAM_ENERGY_STATUS, cpu = package.cores[0].threads[0].id) & 0xFFFFFFFF

    start = time.time()
    time.sleep(period)
    delta = time.time() - start

    consumption = 0.0
    for package in topology.packages:
        new_energy_dram = aux_msr.read_msr(aux_msr.INTEL_DRAM_ENERGY_STATUS, cpu = package.cores[0].threads[0].id)
        if new_energy_dram < energy_dram[package.id]:
            new_energy_dram += 0x100000000
        consumption += (new_energy_dram - energy_dram[package.id]) * rapl_energy_dram / delta

    return consumption


def get_memory_consumption_dummy(period=1):
    """NO CONSUMPTION (do not know how to detect correct consumption of memory)"""
    return 0.0


def gen_memory_consumption_func():
    """Choose function to calculate memory consumption"""
    if aux_arch.get_cpu_vendor() == aux_arch.EVendor.INTEL:
        have_msr_status, have_msr_warn = aux_msr.have_msr()
        if not have_msr_status:
            print aux_utils.red_text(have_msr_warn)
            return get_memory_consumption_dummy
        return get_memory_consumption_intel_msr
    else:
        print aux_utils.red_text('WARN: no memory consumption method found, using dummy ...')
        return get_memory_consumption_dummy

