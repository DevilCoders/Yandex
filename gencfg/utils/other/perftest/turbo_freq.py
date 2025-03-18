"""Detect turbo freq levels"""

import struct

import aux_utils
import aux_msr
import aux_arch


def get_turbo_freq_intel():
    """Get turbo freq levels for Intel processors"""

    arr = struct.pack("<Q", aux_msr.read_msr(aux_msr.INTEL_TURBO_RATIO_LIMIT_RATIOS))

    result = []
    for i in arr:
            result.append( struct.unpack('B',i)[0] * 100 )
    result = sum(map(lambda x: [x, x], result), [])
    if len(result) < aux_arch.get_cpu_count():
        result.extend([result[-1]] * (aux_arch.get_cpu_count() - len(result)))

    return result


def get_turbo_freq_dummy():
    """Dummy turbo freq (do not know how to detect it)"""
    return [0] * aux_arch.get_cpu_count()


@aux_utils.memoize
def get_turbo_freq():
    if aux_arch.get_cpu_vendor() == aux_arch.EVendor.INTEL:
        have_msr_status, have_msr_warn = aux_msr.have_msr()
        if not have_msr_status:
            print aux_utils.red_text(have_msr_warn)
            return get_turbo_freq_dummy()
        return get_turbo_freq_intel()
    else:
        print aux_utils.red_text('WARN: no turbo freq detected, using dummy ...')
        return get_turbo_freq_dummy()
