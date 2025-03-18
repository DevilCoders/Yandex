"""Detect cpu topology utils"""

from collections import defaultdict
import copy
import re

from aux_utils import indent, memoize, have_program, run_command, red_text
from aux_arch import get_cpu_count


class TCpuThread(object):
    """Cpu core thread"""
    def __init__(self, id_, core):
        self.id = id_
        self.core = core
    def __str__(self):
        return '{}'.format(str(self.id))


class TCpuCore(object):
    """Physical cpu core"""
    def __init__(self, id_, package):
        self.id = id_
        self.package = package
        self.threads = []

    def as_dict(self):
        return [x.id for x in self.threads]

    def __str__(self):
        return 'Core{}: {}'.format(self.id, ' '.join(str(x) for x in self.threads))


class TCpuPackage(object):
    """Cpu package"""
    def __init__(self, id_, topology):
        self.id = id_
        self.topology = topology
        self.cores = []

    @property
    def threads(self):
        return sum((x.threads for x in self.cores), [])

    def as_dict(self):
        return {x.id: x.as_dict() for x in self.cores}

    def __str__(self):
        return 'Package{}:\n{}'.format(self.id, indent(self.cores))


class TCpuTopology(object):
    """Cpu topology object"""

    def __init__(self):
        self.packages = []

    @property
    def threads(self):
        return sum((x.threads for x in self.packages), [])

    @property
    def cores(self):
        return sum((x.cores for x in self.packages), [])

    def as_dict(self):
        return {x.id: x.as_dict() for x in self.packages}

    def __str__(self):
        return 'Topology:\n{}' .format(indent(self.packages))


class TNumaNode(object):
    """Numa node"""
    def __init__(self, id_, threads):
        self.id = id_
        self.threads = copy.copy(threads)

    def __str__(self):
        return 'NumaNode{}: {}'.format(self.id, ' '.join(str(x) for x in self.threads))


class TNumaTopology(object):
    """Numa topology"""

    def __init__(self, cpu_topology):
        self.numa_nodes = []
        self.numa_distances = {}
        if not have_program('numactl'):
            print red_text('WARN: util <numactl> not found (try <apt-get install numactl>), numa configuration is not detected')
            self.numa_nodes.append(TNumaNode(0, cpu_topology.threads))
            self.numa_distances[(0, 0)] = 1
        else:
            data = run_command(['numactl', '--hardware'])[0].split('\n')
            data = [x.strip() for x in data]
            # create numa nodes
            for line in data:
                m = re.match('node (\d+) cpus: (.*)', line)
                if m:
                    numa_id = int(m.group(1))
                    thread_ids = [int(x) for x in m.group(2).split()]
                    threads = [x for x in cpu_topology.threads if x.id in thread_ids]
                    self.numa_nodes.append(TNumaNode(numa_id, threads))
            # create distances
            index = data.index('node distances:')
            for node1_id in xrange(len(self.numa_nodes)):
                node1_line = data[index + 2 + node1_id]
                distances = [int(x) for x in node1_line.split()[1:]]
                for node2_id, distance in enumerate(distances):
                    self.numa_distances[(node1_id, node2_id)] = distance

        # check for <vmotouch>
        if not have_program('vmtouch'):
            print red_text('WARN: util <vmtouch> not found (try <apt-get install mtouch>)')

    def __str__(self):
        result = 'NumaTopology:\n{}'.format(indent(self.numa_nodes))
        return result


@memoize
def get_cpu_topology():
    topology_as_dict = defaultdict(lambda: defaultdict(list))
    for cpu in range(get_cpu_count()):
        package = int(open('/sys/devices/system/cpu/cpu%d/topology/physical_package_id' % cpu).read())
        core = int(open('/sys/devices/system/cpu/cpu%d/topology/core_id' % cpu).read())
        topology_as_dict[package][core].append(cpu)

    topology = TCpuTopology()
    for package_id in sorted(topology_as_dict.keys()):
        package = TCpuPackage(package_id, topology)
        package_as_dict = topology_as_dict[package_id]
        for core_id in sorted(package_as_dict.keys()):
            core = TCpuCore(core_id, package)
            for thread_id in package_as_dict[core_id]:
                core.threads.append(TCpuThread(thread_id, core))
            package.cores.append(core)
        topology.packages.append(package)

    return topology


@memoize
def get_numa_topology():
    return TNumaTopology(get_cpu_topology())
