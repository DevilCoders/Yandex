"""Detect configuration of memory/disk/motherboard/..."""

from collections import defaultdict
import json
import os
import re

import aux_utils
import aux_arch

from consumption import gen_package_consumption_func, gen_memory_consumption_func
from eff_freq import gen_effective_freq_func
from turbo_boost import gen_set_turbo_boost
from turbo_freq import get_turbo_freq
from cpu_topology import get_cpu_topology, get_numa_topology


class TConfiguration(object):
    """Hardware configuration of tested machine as object"""

    def __init__(self):
        # detect cpu configuration
        self.instruction_set = aux_arch.get_instruction_set()
        self.vendor = aux_arch.get_cpu_vendor()
        self.vendor_arch = aux_arch.get_cpu_vendor_arch()
        self.cpu_model = aux_arch.get_cpu_model()
        self.cpu_topology = get_cpu_topology()
        self.numa_topology = get_numa_topology()
        self.ncpu = aux_arch.get_cpu_count()
        self.turbo_freq = get_turbo_freq()

        # detect pereiphery configuration
        self.baseboard = self.detect_baseboard()
        self.memory = self.detect_memory()
        self.disks = self.detect_disks()

        # detect functions for modification cpu state, getting consumption, ...
        self.package_consumption_func = gen_package_consumption_func()
        self.memory_consumption_func = gen_memory_consumption_func()
        self.effective_freq_func = gen_effective_freq_func()
        self.set_turbo_boost_func = gen_set_turbo_boost()

    def detect_baseboard(self):
        """Detect motherboard of tested machine"""
        def filter_comments(data):
            """Filter out strings with comments"""
            return '\n'.join([x for x in data.split('\n') if not x.startswith('#')])

        if not aux_utils.have_program('dmidecode'):
            print aux_utils.red_text('WARN: util <dmidecode> not found (try <apt-get install dmidecode>), motherboard is undetected')
            return 'Unknown'
        if not os.access('/dev/mem', os.R_OK):
            print aux_utils.red_text('WARN: util <dmidecode> requires read access to </dev/mem> (run script as <root> to avoid this problem)')
            return 'Unknown'

        baseboard_manufacturer = filter_comments(aux_utils.run_command(['dmidecode', '-s', 'baseboard-manufacturer'])[0]).strip()
        baseboard_product_name = filter_comments(aux_utils.run_command(['dmidecode', '-s', 'baseboard-product-name'])[0]).strip()
        baseboard_version = filter_comments(aux_utils.run_command(['dmidecode', '-s', 'baseboard-version'])[0]).strip()

        return '{} {} {}'.format(baseboard_manufacturer, baseboard_product_name, baseboard_version)

    def detect_memory(self):
        """Detect memory modules installed"""
        if not aux_utils.have_program('dmidecode'):
            print aux_utils.red_text('WARN: util <dmidecode> not found (try <apt-get install dmidecode>), memory is undetected')
            return 'Unknown'
        if not os.access('/dev/mem', os.R_OK):
            print aux_utils.red_text('WARN: util <dmidecode> requires read access to </dev/mem> (run script as <root> to avoid this problem)')
            return 'Unknown'

        memory_info = aux_utils.run_command(['dmidecode', '-t', 'memory'])[0].strip()
        memory_info = re.findall('Memory Device[\W\w]*?Size: (.*) (MB|GB)[\W\w]*?Speed: (.*) MHz[\W\w]*?Manufacturer: (.*)',
                                 memory_info)
        memory_info = map(lambda (sz, mult, speed, manufacturer): '%sGb %s %sMHz' % (int(sz) / 1024 if mult == 'MB' else int(sz), manufacturer, speed),
                          memory_info)
        if len(list(set(memory_info))) > 1:
            print aux_utils.red_text('INFO: Found memory of 2 different types: {}'.format(' '.join(map(lambda x: '<{}>'.format(x), set(memory_info)))))
        memory_info_counts = defaultdict(int)
        for elem in memory_info:
            memory_info_counts[elem] += 1
        memory_info = ",".join(map(lambda x: "%s x %s" % (x, memory_info_counts[x]), sorted(memory_info_counts.keys())))

        return re.sub('  +', ' ', memory_info)

    def detect_disks(self):
        """Detect disks installed"""

        # check disks on Cavium test machine
        if self.instruction_set == aux_arch.EInstructionSet.ARM8:  # lshw hangs on cavium test machine
            if not aux_utils.have_program('lsblk'):
                print aux_utils.red_text('WARN: util <lsblk> not found (try <apt-get install util-linux>), disks is undetected')
                return 'Unknown'
            data = aux_utils.run_command(['lsblk', '-io', 'MODEL', '-J'])[0].strip()
            data = json.loads(data)
            disks_info = (x['model'] for x in data['blockdevices'] if x['model'] is not None)
            disks_info = ', '.join(disks_info)
            return disks_info

        # generic test disks
        if not aux_utils.have_program('lshw'):
            print aux_utils.red_text('WARN: util <lshw> not found (try <apt-get install lshw>), disks is undetected')
            return 'Unknown'
        disks_info = aux_utils.run_command(['lshw', '-class', 'disk', '-class', 'disk', '-short', '-json'])[0].strip()
        disks_info = re.findall('disk\s+(.*)', disks_info)
        disks_info = map(lambda x: '%s x %s' % (x, len(filter(lambda y: y == x, disks_info))), set(disks_info))
        disks_info = ', '.join(disks_info)

        return re.sub('  +', ' ',  disks_info)

    def __str__(self):
        d = dict(
            instruction_set = self.instruction_set,
            vendor = self.vendor,
            vendor_arch = self.vendor_arch,
            cpu_model = self.cpu_model,
            cpu_count = self.ncpu,
            cpu_topology = aux_utils.indent(str(self.cpu_topology), '                   '),
            numa_topology = aux_utils.indent(str(self.numa_topology), '                   '),
            motherboard = self.baseboard,
            memory = self.memory,
            disks = self.disks,
            package_consumption_func = self.package_consumption_func.__doc__,
            memory_consumption_func = self.memory_consumption_func.__doc__,
            effective_freq_func = self.effective_freq_func.__doc__,
            turbo_boost_func = self.set_turbo_boost_func.__doc__
        )

        result = ('Hw:\n'
                  '   Cpu:\n'
                  '       InstructionSet: {instruction_set}\n'
                  '       Vendor: {vendor}\n'
                  '           Arch: {vendor_arch}\n'
                  '               Model:\n'
                  '                   Name: {cpu_model}\n'
                  '                   Ncpu: {cpu_count}\n'
                  '{cpu_topology}\n'
                  '{numa_topology}\n'
                  '   Motherboard: {motherboard}\n'
                  '   Memory: {memory}\n'
                  '   Disks: {disks}\n'
                  'Funcs:\n'
                  '    Consumption:\n'
                  '        Package: {package_consumption_func}\n'
                  '        Memory: {memory_consumption_func}\n'
                  '    Effective freq: {effective_freq_func}\n'
                  '    TB Switch: {turbo_boost_func}\n'
            ).format(**d)

        return result


@aux_utils.memoize
def get_configuration():
    return TConfiguration()
