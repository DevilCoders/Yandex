
from copy import deepcopy
from interface import Interface


class InterfaceUtils(object):
    def __init__(self, network_info):
        self._network_info = network_info

    def generate_plain_interface(self, fqdn, opts, skip_ip4=False):
        ips = self._network_info.get_ip_addresses(fqdn)
        if not ips:
            raise ValueError('cannot resolve fqdn to any ip address')
        if skip_ip4:
            ips['ip4'] = None
        nets = self._network_info.get_networks(ips)
        main_opts = deepcopy(opts)
        secondary_opts = None
        if nets['net4']:
            main_opts.update(nets['net4'])
            if main_opts.get('postup4', False):
                postup = main_opts.setdefault('postup', [])
                postup.extend(main_opts['postup4'])
            if main_opts.get('predown4', False):
                predown = main_opts.setdefault('predown', [])
                predown.extend(main_opts['predown4'])
            if nets['net6']:
                secondary_opts = deepcopy(opts)
                if secondary_opts.get('postup6', False):
                    postup = secondary_opts.setdefault('postup', [])
                    postup.extend(main_opts['postup6'])
                if secondary_opts.get('predown6', False):
                    predown = secondary_opts.setdefault('predown', [])
                    predown.extend(secondary_opts['predown6'])
                secondary_opts['do_ethtool'] = False
                secondary_opts['auto'] = False
                secondary_opts['ya_netcfg_disable'] = False
                secondary_opts['ya_netcfg_fb_disable'] = True
                secondary_opts.update(nets['net6'])
        else:
            main_opts['ya_netcfg_disable'] = False
            main_opts['ya_netcfg_fb_disable'] = True
            if main_opts.get('postup6', False):
                postup = main_opts.setdefault('postup', [])
                postup.extend(main_opts['postup6'])
            if main_opts.get('predown6', False):
                predown = main_opts.setdefault('predown', [])
                predown.extend(main_opts['predown6'])
            main_opts.update(nets['net6'])
        result = [Interface.from_params(self._network_info, main_opts)]
        if secondary_opts:
            result.append(Interface.from_params(
                self._network_info, secondary_opts))
        return result

    def get_loopbacks_by_ip_version(self, fqdn, ip_version):
        result = []
        ip_addresses = self._network_info.get_ip_addresses(fqdn)
        if not ip_addresses:
            raise ValueError('cannot resolve fqdn %s' % fqdn)
        ips = []
        if ip_addresses['ip4']:
            ips.append(ip_addresses['ip4'])
        if ip_addresses['ip6']:
            ips.append(ip_addresses['ip6'])

        for ip in ips:
            loopbacks = self._network_info.slb(ip)
            for l in loopbacks:
                address = l['vip']
                if self._network_info.is_ip(address) == ip_version:
                    if not filter(lambda lb: lb['vip'] == l['vip'], result):
                        result.append(l)

        return result
