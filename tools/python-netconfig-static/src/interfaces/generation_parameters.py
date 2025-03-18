class GenerationParameters(object):
    def __init__(self, options, host_info):
        self.fqdn = options.fqdn

        self.output_format = options.output_format

        if host_info is None:
            raise Exception('No data from conductor about ' + self.fqdn)

        self._host_groups = host_info['host']['groups']
        self._host_tags = host_info['host']['tags']
        host_params = self.params_by_tags(self.host_tags)

        self.do10 = host_params['ipvs']
        if options.do10:
            self.do10 = True

        self.nojumbo = options.nojumbo or host_params['nojumbo']

        self.mtu = options.mtu
        if self.mtu == 0:
            self.mtu = 1450 if self.nojumbo else 8950

        self.default_iface = options.iface

        self.bridged = host_params['bridged']
        if options.bridged:
            self.bridged = True

        self.virtual_host = host_params['virtual']
        if options.virtual:
            self.virtual_host = True

        self.multiqueue = host_params['multiqueue']
        if options.multiqueue:
            self.multiqueue = True
        # host using multiqueue virtIO network is bound to be virtual
        if self.multiqueue:
            self.virtual_host = True

        self.fb_iface = options.fastbone
        if self.virtual_host and self.fb_iface is None:
            self.fb_iface = 'eth1'

        self.no_antimartians = host_params['no_antimartians']
        if options.noantimartians:
            self.no_antimartians = True

        self.narod_misc_dom0_iface = options.narodmiscdom0
        self.no_lo = options.nolo

        self.fb_bridged = host_params['fb_bridged']

        self.bonding = options.bonding

        self.use_tables = options.use_tables

        self.skip_ip4 = 'ipip6' in self.host_tags

    def params_by_tags(self, tags):
        opt_names = dict(
            bridged=['openvz_host', 'openvz_fb_host', 'lxc_host', 'kvm_host'],
            fb_bridged=['openvz_fb_host', 'kvm_fb_host'],
            virtual='virtual_host',
            multiqueue='virtio_multiqueue',
            nojumbo='nojumbo',
            ipvs='ipvs',
            no_antimartians='no_antimartians'
        )
        opts = {}
        for o in opt_names.keys():
            opts[o] = False
            tag_names = opt_names[o]
            if not type(tag_names) == list:
                tag_names = [tag_names]
            for tag_name in tag_names:
                if tag_name in tags:
                    opts[o] = True
                    break
        if "macvlan" in tags and opts.get("bridged", False):
            opts["bridged"] = False
        return opts

    def _get_fqdn(self):
        return self._fqdn

    def _set_fqdn(self, value):
        self._fqdn = value
    fqdn = property(_get_fqdn, _set_fqdn)

    def _get_do10(self):
        return self._do10

    def _set_do10(self, value):
        self._do10 = value
    do10 = property(_get_do10, _set_do10)

    def _get_mtu(self):
        return self._mtu

    def _set_mtu(self, value):
        self._mtu = value
    mtu = property(_get_mtu, _set_mtu)

    def _get_nojumbo(self):
        return self._nojumbo

    def _set_nojumbo(self, value):
        self._nojumbo = value
    nojumbo = property(_get_nojumbo, _set_nojumbo)

    def _get_default_iface(self):
        return self._default_iface

    def _set_default_iface(self, value):
        self._default_iface = value
    default_iface = property(_get_default_iface, _set_default_iface)

    def _get_bridged(self):
        return self._bridged

    def _set_bridged(self, value):
        self._bridged = value
    bridged = property(_get_bridged, _set_bridged)

    def _get_virtual_host(self):
        return self._virtual_host

    def _set_virtual_host(self, value):
        self._virtual_host = value
    virtual_host = property(_get_virtual_host, _set_virtual_host)

    def _get_multiqueue(self):
        return self._multiqueue

    def _set_multiqueue(self, value):
        self._multiqueue = value
    multiqueue = property(_get_multiqueue, _set_multiqueue)

    def _get_fb_iface(self):
        return self._fb_iface

    def _set_fb_iface(self, value):
        self._fb_iface = value
    fb_iface = property(_get_fb_iface, _set_fb_iface)

    def _get_no_antimartians(self):
        return self._no_antimartians

    def _set_no_antimartians(self, value):
        self._no_antimartians = value
    no_antimartians = property(_get_no_antimartians, _set_no_antimartians)

    def _get_narod_misc_dom0(self):
        return self._narod_misc_dom0

    def _set_narod_misc_dom0(self, value):
        self._narod_misc_dom0 = value
    narod_misc_dom0 = property(_get_narod_misc_dom0, _set_narod_misc_dom0)

    def _get_no_lo(self):
        return self._no_lo

    def _set_no_lo(self, value):
        self._no_lo = value
    no_lo = property(_get_no_lo, _set_no_lo)

    def _get_bonding(self):
        return self._bonding

    def _set_bonding(self, value):
        self._bonding = value
    bonding = property(_get_bonding, _set_bonding)

    def _get_use_tables(self):
        return self._use_tables

    def _set_use_tables(self, value):
        self._use_tables = value
    use_tables = property(_get_use_tables, _set_use_tables)

    def _get_skip_ip4(self):
        return self._skip_ip4

    def _set_skip_ip4(self, value):
        self._skip_ip4 = value
    skip_ip4 = property(_get_skip_ip4, _set_skip_ip4)

    @property
    def host_tags(self):
        return self._host_tags

    @property
    def host_groups(self):
        return self._host_groups
