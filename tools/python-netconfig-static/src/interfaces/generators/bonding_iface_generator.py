
from ..interface import Interface


class BondingIfaceGenerator(object):
    def generate(self, parameters, iface_utils, network_info, interfaces):
        if 'bonding' not in parameters.host_tags:
            return
        if not parameters.bonding:
            raise RuntimeError(
                "Tag bonding present but no --bond-interfaces option supplied")

        parameters.default_iface = 'bond0'
        names = parameters.bonding.split()
        if len(names) != 2:
            raise ValueError(
                "Invalid --bond-interfaces value, should be `iface1 iface2'")

        for name in names:
            iface = Interface(network_info)
            iface.name = name
            iface.auto = True
            iface.itype = "manual"
            iface.do_default = False
            iface.do_ethtool = False
            iface.bond_master = 'bond0'
            interfaces.add_interface(iface)

        iface = Interface(network_info)
        iface.name = "bond0"
        iface.itype = "static"
        if parameters.bridged:
            iface.do_default = False
            iface.itype = "manual"
        iface.do_ethtool = False
        iface.add_preup('modprobe bonding')
        for name in names:
            iface.add_preup('ifconfig %s mtu %s ; ethtool -K %s tso off; if lsmod | grep -qE "e1000|igb"; then ethtool -G %s rx 4096; ethtool -G %s tx 4096; fi; true;' %
                            (name, parameters.mtu, name, name, name))
        iface.add_preup(
            "ifconfig bond0 up ; ifconfig bond0 mtu %s" % parameters.mtu)
        iface.bond_mode = '802.3ad'
        iface.bond_miimon = '100'
        iface.bond_slaves = parameters.bonding
        iface.add_postdown("modprobe -r bonding")
        iface.add_postup("sleep 3")
        interfaces.add_interface(iface)
