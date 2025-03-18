"""Detection of vendor and cpu model arch"""

import multiprocessing
import re

import aux_utils


class EInstructionSet(object):
    """Separate basesearch/executor build for different instuctions sets"""
    X86_64 = "x86_64"  # default X86_64/Intel64 instruction set
    ARM8 = "arm8"  # aarch64 architecture for ARM processors
    POWER8 = "power8"  # ppc64le for IBM POWER8+ processors
    UNKNOWN = "unknown"


class EVendor(object):
    """Cpu vendor"""
    AMD = 'amd'
    INTEL = 'intel'
    IBM = 'ibm'
    UNKNOWN = 'Unknown'


class EVendorArch(object):
    """Cpu vendor uarch"""
    # AMD uarch list
    OPTERON = 'Opteron'
    EPYC = 'EPYC'
    # Intel uarch list
    WESTMERE = 'Westmere'
    SANDYBRIDGE = 'SandyBridge'
    IVYBRIDGE = 'IvyBridge'
    SKYLAKE = 'Skylake'
    # IBM uarch list
    POWER8 = 'POWER8'
    # Other uarch
    UNKNOWN = 'Unknown'


@aux_utils.memoize
def get_cpu_count():
    """Calculate cpu count once"""
    return multiprocessing.cpu_count()


@aux_utils.memoize
def get_instruction_set():
    """Detect instruction set of current machine to find, which binary should be executed"""
    if aux_utils.have_program('lscpu'):
        data = aux_utils.run_command('lscpu')[0].strip()
        m = re.search('Architecture:\s+(.*)', data)
        if m:
            if m.group(1) == 'x86_64':
                return EInstructionSet.X86_64
            elif m.group(1) == 'aarch64':
                return EInstructionSet.ARM8
            elif m.group(1) == 'ppc64le':
                return EInstructionSet.POWER8

    if get_cpu_vendor() in (EVendor.INTEL, EVendor.AMD):
        return EInstructionSet.X86_64

    return EInstructionSet.UNKNOWN


@aux_utils.memoize
def get_cpu_vendor():
    """Calculate cpu vendor (needed for correct setup of TB)"""
    MAPPING = {
        'GenuineIntel': EVendor.INTEL,
        'AuthenticAMD': EVendor.AMD,
    }

    # detect from cpuinfo
    cpu_info_data = open('/proc/cpuinfo').read()
    m = re.search('vendor_id\s+:\s+(.*)', cpu_info_data)
    if m:
        return MAPPING[m.group(1)]
    m = re.search('cpu\s+:\s+POWER8', cpu_info_data)
    if m:
        return EVendor.IBM

    return EVendor.UNKNOWN


@aux_utils.memoize
def get_cpu_vendor_arch():
    """Calculate arch or generation of model"""
    CPUID_MAPPING = {
        EVendor.AMD : {  # mapping of (famliy_id, model_id) -> arch name for AMD
            (16, 9): EVendorArch.OPTERON,
            (23, 1): EVendorArch.EPYC,
        },
        EVendor.INTEL : {  # mapping of (family_id, model_id) -> arch name for Intel
            (6, 85): EVendorArch.SKYLAKE,
            (6, 79): EVendorArch.SKYLAKE,
            (6, 62): EVendorArch.IVYBRIDGE,
            (6, 45): EVendorArch.SANDYBRIDGE,
            (6, 42): EVendorArch.SANDYBRIDGE,
            (6, 44): EVendorArch.WESTMERE,
            (6, 37): EVendorArch.WESTMERE,
            (6, 47): EVendorArch.WESTMERE,
        },
    }

    cpu_info_data = open('/proc/cpuinfo').read()

    # detect AMD/Intel
    if get_cpu_vendor() in (EVendor.INTEL, EVendor.AMD):
        mapping = CPUID_MAPPING[get_cpu_vendor()]
        m_family = re.search('cpu family\s+:\s+(.*)', cpu_info_data)
        m_model = re.search('model\s+:\s+(.*)', cpu_info_data)
        if (m_family is not None) and (m_model is not None):
            family_id = int(m_family.group(1))
            model_id = int(m_model.group(1))
            if (family_id, model_id) in mapping:
                return mapping[(family_id, model_id)]

    # detect IBM from cpu info
    if get_cpu_vendor() == EVendor.IBM:
        m = re.search('cpu\s+:\s+POWER', cpu_info_data)
        if m:
            return EVendorArch.POWER8

    return EVendorArch.UNKNOWN


@aux_utils.memoize
def get_cpu_model():
    """Get cpu model as human readable string"""
    data = open('/proc/cpuinfo').read()
    m = re.search('model name\s+:\s+(.*)', data)
    if m:
        return m.group(1)

    return 'Unknown'
