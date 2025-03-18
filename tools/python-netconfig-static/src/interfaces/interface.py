
import pprint
from copy import copy
from route import Route
from rule import Rule


class Interface(object):
    def __init__(self, network_info=None):
        if not network_info:
            raise ValueError('pass network_info to construct interface')
        self._network_info = network_info
        self._auto = True
        self._type = "static"
        self._default_mtu = 1450
        self._do_default = True
        self._do_ethtool = True
        self._jumboroute = True
        self._comments = []
        self._jumbo_nets = []
        self._preup = []
        self._up = []
        self._down = []
        self._postup = []
        self._predown = []
        self._postdown = []
        self._render_bridge_params = True
        self._name = None
        self._address = None
        self._netmask = None
        self._network = None
        self._broadcast = None
        self._mtu = None
        self._default_advmss = None
        self._gateway = None
        self._vlan_raw_device = None
        self._bridge_ports = None
        self._bond_mode = None
        self._bond_miimon = None
        self._bond_slaves = None
        self._bond_master = None
        self._vlan = None
        self._is_fastbone = False
        self._use_tables = False
        self._family = None
        self._privext = None
        self._ya_netcfg_disable = True
        self._ya_netcfg_fb_disable = False
        self._ya_netcfg_fb_iface = None

        self._valid_attributes_ = []
        props = vars(Interface)
        for prop in props:
            if props[prop].__class__ == property and props[prop].fset:
                self._valid_attributes_.append(prop)

    def update(self, opts):
        for attr in self._valid_attributes_:
            if isinstance(opts, Interface):
                if not getattr(opts, attr) is None:
                    self.__setattr__(attr, copy(getattr(opts, attr)))
            elif attr in opts and not opts[attr] is None:
                self.__setattr__(attr, copy(opts[attr]))
        return self

    def is_bridge(self):
        return self.name[0:2] == "br"

    def add_comment(self, comment):
        self._comments.append(comment)

    def add_comments(self, comments):
        self._comments += comments

    def add_preup(self, preup):
        self._preup.append(preup)

    def add_up(self, up):
        self._up.append(up)

    def add_down(self, down):
        self._down.append(down)

    def add_postups(self, postups):
        self._postup += postups

    def add_postup(self, postup):
        self._postup.append(postup)

    def add_predown(self, predown):
        self._predown.append(predown)

    def add_predowns(self, predowns):
        self._predown += predowns

    def add_postdown(self, postdown):
        self._postdown.append(postdown)

    def generate_actions(self):
        family = self._network_info.is_ip(self.address)
        gen_preup = []
        gen_postup = []
        gen_predown = []
        gen_postdown = []

        if self.do_default:
            route_replace = Route().replace().ip_version(
                family).default().via(self.gateway).mtu(self.default_mtu)
            if self.default_advmss is not None:
                route_replace.advmss(self.default_advmss)
            gen_postup.append(route_replace)

        if self.default_mtu != self.mtu and self.jumboroute:
            if self.use_tables and self.is_fastbone:
                route_replace = Route().replace().ip_version(family).default().via(
                    self.gateway).mtu(self.encap_mtu).table(self.vlan)
                gen_postup.append(route_replace)
                for jn in self.jumbo_nets:
                    rule_add = Rule().add().ip_version(family).to_addr(jn).lookup(self.vlan)
                    gen_postup.append(rule_add)
            else:
                for jn in self.jumbo_nets:
                    route_replace = Route().replace().ip_version(
                        family).route(jn).via(self.gateway).mtu(self.encap_mtu)
                    gen_postup.append(route_replace)

        vz_postup = '[ -e /tmp/#IFACE#veth ] && ( for iface in `cat /tmp/#IFACE#veth`;' + \
                    'do export VEID=`echo $iface | sed -r \'s/^veth([0-9]+)\\..*$/\\1/\'`; ' + \
                    '[ -x /usr/bin/vznetaddbr -a x`vzlist -H -o status $VEID` = xrunning ] ' + \
                    '&& /usr/bin/vznetaddbr init veth $iface; done; rm -f /tmp/#IFACE#veth ); true'
        vz_predown = 'brctl show #IFACE# | awk \'$NF ~ /^veth/ { print $NF }\' > /tmp/#IFACE#veth; true'
        ethtool_tune = 'ethtool -K #IFACE# tso off; if lsmod | grep -qE "e1000|igb"; ' + \
                       'then ethtool -G #IFACE# rx 4096 tx 4096; fi; true'

        if self.bridge_ports:
            gen_preup.append("ifconfig " + self.bridge_ports +
                             " mtu " + str(self.mtu) + "; true")
            gen_postup.append(vz_postup.replace("#IFACE#", self.name))
            gen_predown.append(vz_predown.replace("#IFACE#", self.name))
            if self.do_ethtool:
                gen_preup.append(ethtool_tune.replace(
                    "#IFACE#", self.bridge_ports))
        else:
            if self.do_ethtool:
                gen_preup.append(ethtool_tune.replace("#IFACE#", self.name))

        if self.family == 6 and self.mtu is not None:
            self.up = ['ifconfig {} up mtu {}; true'.format(
                self.name, self.mtu)] + self.up
            self.mtu = None

        self.preup = gen_preup + self.preup
        self.postup = gen_postup + self.postup
        self.predown = gen_predown + self.predown
        self.postdown = gen_postdown + self.postdown

    def __str__(self):
        import output.formats
        out = output.formats.Output()
        output.formats.Debian.interface(self, out)
        return out.getvalue()

    def __repr__(self):
        return "<%s: %s>" % (self.__class__.__name__, pprint.pformat(dict(filter(lambda (k, v): k[0] == '_', self.__dict__.items()))))

    def _get_family(self):
        return getattr(self, '_family', self._network_info.is_ip(self.address))

    def _set_family(self, value):
        self._family = value
    family = property(_get_family, _set_family)

    def _get_name(self):
        return self._name

    def _set_name(self, value):
        self._name = value
    name = property(_get_name, _set_name)

    def _get_itype(self):
        return self._type

    def _set_itype(self, value):
        self._type = value
    itype = property(_get_itype, _set_itype)

    def _get_address(self):
        return self._address

    def _set_address(self, value):
        self._address = value
    address = property(_get_address, _set_address)

    def _get_netmask(self):
        return self._netmask

    def _set_netmask(self, value):
        self._netmask = value
    netmask = property(_get_netmask, _set_netmask)

    def _get_network(self):
        return self._network

    def _set_network(self, value):
        self._network = value
    network = property(_get_network, _set_network)

    def _get_broadcast(self):
        return self._broadcast

    def _set_broadcast(self, value):
        self._broadcast = value
    broadcast = property(_get_broadcast, _set_broadcast)

    def _get_comments(self):
        return self._comments

    def _set_comments(self, comments):
        if comments is None:
            comments = []
        if type(comments) == str:
            comments = [comments]
        self._comments = comments
    comments = property(_get_comments, _set_comments)

    def _get_auto(self):
        return self._auto

    def _set_auto(self, value):
        self._auto = value
    auto = property(_get_auto, _set_auto)

    def _get_mtu(self):
        return self._mtu

    def _set_mtu(self, value):
        self._mtu = value
    mtu = property(_get_mtu, _set_mtu)

    @property
    def encap_mtu(self):
        return self.mtu - 50  # see CONDUCTOR-783 for -50

    def _get_default_mtu(self):
        return self._default_mtu

    def _set_default_mtu(self, value):
        self._default_mtu = value
    default_mtu = property(_get_default_mtu, _set_default_mtu)

    def _get_default_advmss(self):
        return self._default_advmss

    def _set_default_advmss(self, value):
        self._default_advmss = value
    default_advmss = property(_get_default_advmss, _set_default_advmss)

    def _get_jumbo_nets(self):
        return self._jumbo_nets

    def _set_jumbo_nets(self, value):
        self._jumbo_nets = value
    jumbo_nets = property(_get_jumbo_nets, _set_jumbo_nets)

    def _get_jumboroute(self):
        return self._jumboroute

    def _set_jumboroute(self, value):
        self._jumboroute = value
    jumboroute = property(_get_jumboroute, _set_jumboroute)

    def _get_gateway(self):
        return self._gateway

    def _set_gateway(self, value):
        self._gateway = value
    gateway = property(_get_gateway, _set_gateway)

    def _get_vlan_raw_device(self):
        return self._vlan_raw_device

    def _set_vlan_raw_device(self, value):
        self._vlan_raw_device = value
    vlan_raw_device = property(_get_vlan_raw_device, _set_vlan_raw_device)

    def _get_do_default(self):
        return self._do_default

    def _set_do_default(self, value):
        self._do_default = value
    do_default = property(_get_do_default, _set_do_default)

    def _get_do_ethtool(self):
        return self._do_ethtool

    def _set_do_ethtool(self, value):
        self._do_ethtool = value
    do_ethtool = property(_get_do_ethtool, _set_do_ethtool)

    def _get_preup(self):
        return self._preup

    def _set_preup(self, value):
        self._preup = value
    preup = property(_get_preup, _set_preup)

    def _get_up(self):
        return self._up

    def _set_up(self, value):
        self._up = value
    up = property(_get_up, _set_up)

    def _get_down(self):
        return self._down

    def _set_down(self, value):
        self._down = value
    down = property(_get_down, _set_down)

    def _get_postup(self):
        return self._postup

    def _set_postup(self, value):
        self._postup = value
    postup = property(_get_postup, _set_postup)

    def _get_postdown(self):
        return self._postdown

    def _set_postdown(self, value):
        self._postdown = value
    postdown = property(_get_postdown, _set_postdown)

    def _get_predown(self):
        return self._predown

    def _set_predown(self, value):
        self._predown = value
    predown = property(_get_predown, _set_predown)

    def _get_bridge_ports(self):
        return self._bridge_ports

    def _set_bridge_ports(self, value):
        self._bridge_ports = value
    bridge_ports = property(_get_bridge_ports, _set_bridge_ports)

    def _get_bond_mode(self):
        return self._bond_mode

    def _set_bond_mode(self, value):
        self._bond_mode = value
    bond_mode = property(_get_bond_mode, _set_bond_mode)

    def _get_vlan(self):
        return self._vlan

    def _set_vlan(self, vlan):
        self._vlan = vlan
    vlan = property(_get_vlan, _set_vlan)

    def _get_is_fastbone(self):
        return self._is_fastbone

    def _set_is_fastbone(self, value):
        self._is_fastbone = value
    is_fastbone = property(_get_is_fastbone, _set_is_fastbone)

    def _get_use_tables(self):
        return self._use_tables

    def _set_use_tables(self, value):
        self._use_tables = value
    use_tables = property(_get_use_tables, _set_use_tables)

    def _get_bond_miimon(self):
        return self._bond_miimon

    def _set_bond_miimon(self, value):
        self._bond_miimon = value
    bond_miimon = property(_get_bond_miimon, _set_bond_miimon)

    def _get_bond_slaves(self):
        return self._bond_slaves

    def _set_bond_slaves(self, value):
        self._bond_slaves = value
    bond_slaves = property(_get_bond_slaves, _set_bond_slaves)

    def _get_bond_master(self):
        return self._bond_master

    def _set_bond_master(self, value):
        self._bond_master = value
    bond_master = property(_get_bond_master, _set_bond_master)

    def _get_render_bridge_params(self):
        return self._render_bridge_params

    def _set_render_bridge_params(self, value):
        self._render_bridge_params = value
    render_bridge_params = property(
        _get_render_bridge_params, _set_render_bridge_params)

    def _get_privext(self):
        return self._privext

    def _set_privext(self, value):
        self._privext = value
    privext = property(_get_privext, _set_privext)

    def _get_ya_netcfg_disable(self):
        return self._ya_netcfg_disable

    def _set_ya_netcfg_disable(self, value):
        self._ya_netcfg_disable = value
    ya_netcfg_disable = property(
        _get_ya_netcfg_disable, _set_ya_netcfg_disable)

    def _get_ya_netcfg_fb_disable(self):
        return self._ya_netcfg_fb_disable

    def _set_ya_netcfg_fb_disable(self, value):
        self._ya_netcfg_fb_disable = value
    ya_netcfg_fb_disable = property(
        _get_ya_netcfg_fb_disable, _set_ya_netcfg_fb_disable)

    def _get_ya_netcfg_fb_iface(self):
        return self._ya_netcfg_fb_iface

    def _set_ya_netcfg_fb_iface(self, value):
        self._ya_netcfg_fb_iface = value
    ya_netcfg_fb_iface = property(
        _get_ya_netcfg_fb_iface, _set_ya_netcfg_fb_iface)

    @staticmethod
    def from_params(network_info, params):
        return Interface(network_info).update(params)
