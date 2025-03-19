import os
import re
import sys
import traceback

import interfaces

from ..interfaces_group import InterfacesGroup
from ..interface_utils import InterfaceUtils
from . import (
    LoopbackIfaceGenerator,
    BondingIfaceGenerator,
    PlainIfaceGenerator,
    TunnelIfaceGenerator,
    AdditionalIfaceGenerator,
    FastboneIfaceGenerator,
    BalancerIfaceGenerator,
    VlanIfaceGenerator,
    CdnIfaceGenerator,
    DockerIfaceGenerator,
    OpenstackIfaceGenerator,
)


class InterfacesGenerator(object):
    PLUGIN_FILE_RE = re.compile(r'(\d+)-[\w\-]+')

    def __init__(self, params, plugin_path):
        self.params = params
        self.plugin_path = plugin_path

    def generate(self, network_info):
        NAME_WEIGHTS = {
            'lo': 1,
            'lo:': 200,
            'eth': 2,
            'dummy': 201,
            'br': 3,
            'ip6tnl0': 20,
            'tunl0': 19,
            'vlan': 10,
            'bond': 5,
        }

        def max_weight(name):
            w = 0
            matched = False
            for k, v in NAME_WEIGHTS.items():
                if k in name and v > w:
                    w = v
                    matched = True
            return matched and w or 100

        def compare_ifaces(iface1, iface2):
            name1 = max_weight(iface1.name)
            name2 = max_weight(iface2.name)
            return cmp(name1, name2)

        iface_utils = InterfaceUtils(network_info)

        generators = [
            LoopbackIfaceGenerator(),
            BondingIfaceGenerator(),
            PlainIfaceGenerator(),
            OpenstackIfaceGenerator(),
            TunnelIfaceGenerator(),
            AdditionalIfaceGenerator(),
            FastboneIfaceGenerator(),
            BalancerIfaceGenerator(),
            VlanIfaceGenerator(),
            CdnIfaceGenerator(),
            DockerIfaceGenerator(),
        ]

        interfaces_group = InterfacesGroup()
        for generator in generators:
            generator.generate(self.params, iface_utils, network_info, interfaces_group)

        self.apply_plugins(interfaces_group)
        interfaces = InterfacesGroup()
        interfaces.add_interfaces(sorted(interfaces_group, cmp=compare_ifaces))
        return interfaces

    def apply_plugins(self, interfaces_list):
        if not os.path.exists(self.plugin_path):
            if interfaces.DEBUG:
                print 'plugin path %s does not exist' % self.plugin_path
                print '-' * 20
            return
        if not os.path.isdir(self.plugin_path):
            if interfaces.DEBUG:
                print 'plugin path %s is not a directory' % self.plugin_path
                print '-' * 20
            return
        possible_plugins = [f[:-3] for f in os.listdir(self.plugin_path) if f.endswith('.py')]
        plugins = []
        for f in possible_plugins:
            match = self.PLUGIN_FILE_RE.match(f)
            if match:
                plugins.append((match.group(1), match.group(0)))
            elif interfaces.DEBUG:
                print 'found file %s.py in plugin dir, but it\'s not /\\d+-[\\w\\-]+/' % f
                print '-' * 20
        sys.path.append(self.plugin_path)
        for f in sorted(plugins):
            try:
                plugin = __import__(f[1])
                if hasattr(plugin, 'filter'):
                    if not plugin.filter(self.params):
                        continue
                plugin.process(interfaces_list, self.params)
            except Exception, e:
                if interfaces.DEBUG:
                    print 'failed running plugin %s: %s' % (f[1], e)
                    traceback.print_exc()
                    print '-' * 20
                raise
