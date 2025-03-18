import StringIO
from interfaces.interface_action import InterfaceAction
from interfaces.address import Address
from interfaces.route import Route
from interfaces.rule import Rule


class Output(StringIO.StringIO):
    NEWLINE = "\n"

    def writeln(self, line=''):
        self.write(line)
        self.write(self.NEWLINE)

    def write(self, *args):
        for s in args:
            StringIO.StringIO.write(self, s)
        return self


class Formatter(object):
    out = None

    @classmethod
    def as_string(cls, interfaces_group):
        """
        @type interfaces_group: interfaces.interfaces_group.InterfacesGroup
        """
        result = Output()
        cls.out = result
        for i in interfaces_group:
            result.writeln(cls.interface(i))
        return result.getvalue()

    @classmethod
    def interface(cls, interface, out=None):
        raise NotImplementedError()


class Debian(Formatter):
    @classmethod
    def interface(cls, interface, out=None):
        if not out:
            out = cls.out

        for comment in interface.comments:
            out.writeln("# " + comment)

        if interface.auto:
            out.writeln("auto " + interface.name)

        out.write("iface " + interface.name + " inet")
        if interface.family == 6:
            out.write("6")
        out.writeln(" " + interface.itype)

        if interface.address:
            out.writeln("\taddress " + interface.address)
        if interface.netmask:
            out.writeln("\tnetmask " + interface.netmask)
        if interface.network:
            out.writeln("\tnetwork " + interface.network)
        if interface.broadcast:
            out.writeln("\tbroadcast " + interface.broadcast)
        if interface.mtu and not interface.bridge_ports:
            out.writeln("\tmtu " + str(interface.mtu))
        if interface.vlan_raw_device:
            out.writeln("\tvlan_raw_device " + interface.vlan_raw_device)
        if interface.bond_master:
            out.writeln("\tbond-master " + interface.bond_master)
        if interface.bond_mode:
            out.writeln("\tbond-mode " + interface.bond_mode)
        if interface.bond_miimon:
            out.writeln("\tbond-miimon " + interface.bond_miimon)
        if interface.bond_slaves:
            out.writeln("\tbond-slaves " + interface.bond_slaves)
        if interface.address:
            out.writeln("\tdad-attempts 0")
        if interface.privext in [0, 1, 2]:
            out.writeln("\tprivext " + str(interface.privext))
        if interface.ya_netcfg_disable:
            out.writeln("\tya-netconfig-disable yes")
        if interface.ya_netcfg_fb_disable:
            out.writeln("\tya-netconfig-fb-disable yes")
        if interface.ya_netcfg_fb_iface:
            out.writeln("\tya-netconfig-fb-iface-{} {}".format(*
                                                               interface.ya_netcfg_fb_iface))

        if interface.bridge_ports:
            if interface.render_bridge_params:
                out.writeln("\tbridge_ports " + interface.bridge_ports)
                out.writeln("\tbridge_stp off")

        cls.actions("pre-up", interface.preup, out)
        cls.actions("up", interface.up, out)
        cls.actions("post-up", interface.postup, out)
        cls.actions("pre-down", interface.predown, out)
        cls.actions("down", interface.down, out)
        cls.actions("post-down", interface.postdown, out)

    @classmethod
    def actions(cls, command, actions, out=None):
        for action in actions:
            if isinstance(action, InterfaceAction) and action.get_comment():
                out.write("\t# %s\n" % action.get_comment())
            cls.action(action, command, out)

    @classmethod
    def action(cls, action, command, out=None):
        out.write("\t", command, ' ')
        if isinstance(action, Address):
            cls.address(action, out)
        elif isinstance(action, Route):
            cls.route(action, out)
        elif isinstance(action, Rule):
            cls.rule(action, out)
        else:
            out.write(str(action))
        out.writeln()

    @classmethod
    def address(cls, action, out=None):
        out.write(action.get_sbin_ip(), ' address ')
        out.write(action._action, ' ', action._address)
        out.write(' dev ', action._dev)
        if action._label:
            out.write(' label ', action._label)

    @classmethod
    def route(cls, action, out):
        if action._ip_version == 6:
            head_length = 60
        else:
            head_length = 40

        out.write(action.get_sbin_ip(), ' route ')
        out.write(action._action, ' ', action._route)
        if action._via:
            out.write(' via ', action._via)
        if action._dev:
            out.write(' dev ', action._dev)
        if action._src:
            out.write(' src ', action._src)
        if action._mtu:
            out.write(' mtu ', str(action._mtu))
        if action._advmss:
            out.write(' advmss ', str(action._advmss))
        elif action._mtu:
            out.write(' advmss ', str(action._mtu - head_length))
        if action._table:
            out.write(' table ', str(action._table))
        if action._metric:
            out.write(' metric ', str(action._metric))

    @classmethod
    def rule(cls, action, out):
        out.write(action.get_sbin_ip(), ' rule ')
        out.write(action._action)
        if action._iif:
            out.write(' iif ', action._iif)
        if action._from:
            out.write(' from ', action._from)
        if action._to:
            out.write(' to ', action._to)
        if action._oif:
            out.write(' oif ', action._oif)
        if action._fwmark:
            out.write(' fwmark ', str(action._fwmark))
        if action._lookup:
            out.write(' lookup ', str(action._lookup))
        if action._priority:
            out.write(' priority ', str(action._priority))
        if action._table:
            out.write(' table ', str(action._table))


class Redhat(Formatter):
    @staticmethod
    def extract_addresses(postups, device):
        return [x._address.split('/', 1)[0] for x in postups if isinstance(x, Address) and x._dev == device]

    @classmethod
    def interface(cls, interface, out=None):
        if not out:
            out = cls.out

        if interface.bridge_ports:
            if interface.render_bridge_params:
                out.writeln("DEVICE=" + interface.bridge_ports)
                out.writeln("BRIDGE=br0")
                out.writeln("NM_CONTROLLED=no")
                if interface.auto:
                    out.writeln('ONBOOT=yes')
                if interface.mtu:
                    out.writeln("MTU=" + str(interface.mtu))
                out.writeln()
                out.writeln()

        out.writeln("DEVICE=" + interface.name)
        if interface.bridge_ports:
            if interface.render_bridge_params:
                out.writeln("NM_CONTROLLED=no")
                out.writeln("BOOTPROTO=Bridge")
                out.writeln("DELAY=0")
                out.writeln("STP=no")
        if interface.vlan_raw_device:
            out.writeln('VLAN=yes')
            out.writeln('VLAN_NAME_TYPE=VLAN_PLUS_VID_NO_PAD')
            out.writeln('PHYSDEV=' + interface.vlan_raw_device + '\n')
        if not interface.bridge_ports:
            if interface.itype:
                out.writeln("BOOTPROTO=" + interface.itype)
        if interface.mtu:
            out.writeln("MTU=" + str(interface.mtu))
        if interface.auto:
            out.writeln('ONBOOT=yes')

        # v6-specific
        if interface.family == 6:
            out.writeln("IPV6INIT=yes")
            out.writeln("IPV6_AUTOCONF=no")
            if interface.address:
                out.writeln("IPV6ADDR=" + interface.address)
            if interface.gateway:
                out.writeln("IPV6_DEFAULTGW=" + interface.gateway)
            addrs = cls.extract_addresses(interface.postup, interface.name)
            if addrs:
                out.write("IPV6ADDR_SECONDARIES=\"\\\n")
                for addr in addrs:
                    out.write(addr + " \\\n")
                out.write("\" # end of v6 secondaries\n")

        # v4-specific
        else:
            if interface.address:
                out.writeln("IPADDR=" + interface.address)
            if interface.gateway:
                out.writeln("GATEWAY=" + interface.gateway)
            if interface.netmask:
                out.writeln("NETMASK=" + interface.netmask)

        # rules
        rules = [x for x in interface.postup if isinstance(x, Rule)]
        if rules:
            out.writeln()
            for rule in rules:
                cls.rule(rule, out)
                out.writeln()

        # routes
        routes = [x for x in interface.postup if isinstance(x, Route)]
        if routes:
            out.writeln()
            for route in routes:
                cls.route(route, out)
                out.writeln()

        out.writeln()

    @classmethod
    def rule(cls, rule, out=None):
        out.write('from ', rule._from)
        if rule._lookup:
            out.write(' lookup ', str(rule._lookup))
        if rule._priority:
            out.write(' priority ', str(rule._priority))
        if rule._table:
            out.write(' table ', str(rule._table))

    @classmethod
    def route(cls, route, out=None):
        if route._ip_version == 6:
            head_length = 60
        else:
            head_length = 40

        out.write(route._route)
        if route._via:
            out.write(' via ', route._via)
        if route._dev:
            out.write(' dev ', route._dev)
        if route._src:
            out.write(' src ', route._src)
        if route._mtu:
            out.write(' mtu ', str(route._mtu))
        if route._advmss:
            out.write(' advmss ', str(route._advmss))
        elif route._mtu:
            out.write(' advmss ', str(route._mtu - head_length))
        if route._table:
            out.write(' table ', str(route._table))
