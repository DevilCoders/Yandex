"""Msr manipulations"""

import os
import struct


MSR_FILE = '/dev/cpu/0/msr'
MSR_NO_MODULE_WARN = 'WARN: No msr module loaded (try <modrpbe msr>), unable to execute msr-based commands...'
MSR_NO_PERMISSIONS_WARN = 'WARN: You have no permissions to read/write <{}>'.format(MSR_FILE)


def bit_mask(min_bit, max_bit):
    """Create mask to filter only [min_bit:max_bit]"""
    result = (1 << (max_bit+1)) - 1
    result = (result >> min_bit) << min_bit
    return result

# Intel msr constants (https://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-system-programming-manual-325384.html)
INTEL_TURBO_RATIO_LIMIT_RATIOS = 0x1ad  # Maximum Ratio Limit of Turbo Mode (RW)
INTEL_RAPL_POWER_UNIT = 0x606  # Unit Multipliers used in RAPL Interfaces (R/O)
INTEL_RAPL_POWER_UNIT_ESU_BITS = (8, 12)
INTEL_PKG_ENERGY_STATUS = 0x611  # PKG Energy Status (R/O)
INTEL_DRAM_ENERGY_STATUS = 0x619  # Energy Consumed by DRAM devices
INTEL_MPERF = 0xe7  # Maximum Performance Frequency Clock Count. (RW)
INTEL_APERF = 0xe8  # Actual Performance Frequency Clock Count. (RW)
INTEL_PLATFORM_INFO = 0xce  # ???

# AMD Epyc msr constants (http://support.amd.com/TechDocs/54945_PPR_Family_17h_Models_00h-0Fh.pdf)
AMD_RAPL_PWR_UNIT = 0xC0010299  # Unit Multipliers used in RAPL Interfaces (R/O)
AMD_RAPL_PWR_UNIT_ESU_BITS = (8, 12)
AMD_PKG_ENERGY_STAT = 0xC001029B  # PKG Energy Status (R/O)
AMD_PKG_ENERGY_STAT_BITS = (0, 31)
AMD_MPERF = 0x000000E7  # Maximum Performance Frequency Clock Count. (RW)
AMD_APERF = 0x000000E8  # Actual Performance Frequency Clock Count. (RW)


def have_msr():
    """Check if msr module is loaded and we have correct permissions"""
    if not os.path.exists('/dev/cpu/0/msr'):
        return False, MSR_NO_MODULE_WARN
    if not (os.access('/dev/cpu/0/msr', os.R_OK) and os.access('/dev/cpu/0/msr', os.W_OK)):
        return False, MSR_NO_PERMISSIONS_WARN
    return True, None


def read_msr(msr, cpu = 0, bits=None):
    try:
        f = os.open('/dev/cpu/%d/msr' % (cpu,), os.O_RDONLY)
        os.lseek(f, msr, os.SEEK_SET)
        val = struct.unpack('Q', os.read(f, 8))[0]
        os.close(f)
        if bits is not None:
            min_bit, max_bit = bits
            val &= bit_mask(min_bit, max_bit)
            val >>= min_bit
        return val
    finally:
        try:
            os.close(f)
        except:
            pass
