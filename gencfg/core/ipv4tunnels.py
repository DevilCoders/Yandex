"""Local storage of ipv4 tunnels for groups (GENCFG-1077)"""

import os
import copy
import json
from collections import namedtuple, defaultdict
import netaddr
import time
import datetime

import gencfg
import gaux.aux_utils


class TIpv4Tunnels(object):
    """Storage with mapping (instance -> address)"""

    __slots__ = ('DATA_FILE', 'db', 'pool', 'allocated', 'waiting_removal', 'modified')

    class TIpv4Pool(object):
        """Pool of free addresses"""

        TIpv4LocPool = namedtuple('TIpv4LocPool', ['subnet', 'descr', 'flt', 'free_ips'])

        def __init__(self, jsoned):
            """Initialize pool from json"""
            self.loc_pools = []
            for elem in jsoned:
                subnet = netaddr.IPNetwork(elem['subnet'])
                descr = elem['descr']
                flt = elem['flt']
                if elem['free_ips'] != '':
                    free_ips = [netaddr.IPAddress(x) for x in elem['free_ips'].split(' ')]
                else:
                    free_ips = []
                self.loc_pools.append(self.TIpv4LocPool(subnet=subnet, descr=descr, flt=flt, free_ips=free_ips))

        def alloc(self, instance):
            """Alloc ip for instance"""
            no_ip_pools = []
            for loc_pool in self.loc_pools:
                if eval(loc_pool.flt)(instance):
                    if len(loc_pool.free_ips):
                        return str(loc_pool.free_ips.pop())
                    else:
                        no_ip_pools.append(loc_pool.descr)

            if no_ip_pools:
                raise Exception('Location pools <{}> do not have free ips when allocating for instance <{}>'.format(','.join(no_ip_pools), instance.full_name()))
            else:
                raise Exception('Not found pool for host  <{}> (dc {}, queue {})'.format(instance.host.name, instance.host.dc, instance.host.queue))

        def free(self, ip):
            """Free ip"""
            ip = netaddr.IPAddress(ip)
            for loc_pool in self.loc_pools:
                if ip in loc_pool.subnet:
                    loc_pool.free_ips.append(ip)
                    return
            raise Exception('Ip address <{}> does not belong to pool'.format(ip))

        def get_subnet_descr(self, ip):
            ip = netaddr.IPAddress(ip)
            for loc_pool in self.loc_pools:
                if ip in loc_pool.subnet:
                    return '{} (mask {})'.format(loc_pool.descr, loc_pool.subnet)
            raise Exception('Ip address <{}> does not belong to pool'.format(ip))

        def ip_in_mask_list(self, ip):
            """Check if specified can be stored in our pool"""
            ip = netaddr.IPAddress(ip)
            for loc_pool in self.loc_pools:
                if ip in loc_pool.subnet:
                    return True
            return False

        def to_json(self):
            """Return dict with data"""
            result = []
            for loc_pool in self.loc_pools:
                result.append(dict(subnet=str(loc_pool.subnet), descr=loc_pool.descr, flt=loc_pool.flt, free_ips=' '.join(str(x) for x in sorted(loc_pool.free_ips))))
            return result

        def __str__(self):
            return self.format_stat()

        def format_stat(self, waiting_to_remove=None):
            result = []
            for loc_pool in self.loc_pools:
                line = '<{}> on subnet <{}>: {} free ip left'.format(loc_pool.descr, loc_pool.subnet, len(loc_pool.free_ips))

                if waiting_to_remove is not None:
                    to_remove_list = []
                    for to_remove in waiting_to_remove:
                        ip = to_remove["tunnel_ip"]
                        if ip in loc_pool.subnet:
                            to_remove_list.append(to_remove)

                    line += ", {} ip to remove".format(len(to_remove_list))

                result.append(line)

            return '\n'.join(result)

    def __init__(self, db):
        import gaux.aux_hbf

        self.db = db
        self.modified = False

        if self.db.version < '2.2.37':
            return

        self.DATA_FILE = os.path.join(self.db.PATH, 'ip4tunnels.json')
        jsoned = json.loads(open(self.DATA_FILE).read())

        # initialize pool
        if self.db.version <= '2.2.46':
            self.pool = dict(default=TIpv4Tunnels.TIpv4Pool(jsoned['pool']))
        else:  # GENCFG-2240: multiple pools
            self.pool = {k: TIpv4Tunnels.TIpv4Pool(v) for k, v in jsoned['pool'].iteritems()}

        # initialize instances info
        self.allocated = {}
        for elem in jsoned['allocated']:
            assert(self.db.hosts.has_host(str(elem['host'])))
            if self.db.version <= '2.2.46':
                self.allocated[(elem['host'], elem['port'], 'default')] = elem['tunnel_ip']
            else:
                self.allocated[(elem['host'], elem['port'], elem['pool_name'])] = elem['tunnel_ip']

        # =================================== RX-526 START ==========================================
        self.waiting_removal = []
        for elem in jsoned.get('waiting_removal', []):
            expired_at = int(time.mktime(time.strptime(elem['expired_at'], '%Y-%m-%d %H:%M')))
            self.waiting_removal.append(dict(tunnel_ip=elem['tunnel_ip'], expired_at=expired_at))
        # =================================== RX-526 FINISH =========================================



    def mark_as_modified(self):
        self.modified = True

    def alloc(self, instance, pool_name):
        assert ((instance.host.name, instance.port, pool_name) not in self.allocated)

        tunnel_ip = self.pool[pool_name].alloc(instance)

        self.allocated[(instance.host.name, instance.port, pool_name)] = tunnel_ip

        self.mark_as_modified()

        return tunnel_ip

    def free(self, instance, pool_name):
        assert ((instance.host.name, instance.port, pool_name) in self.allocated)

        tunnel_ip = self.allocated.pop((instance.host.name, instance.port, pool_name))
        # self.mtn_fqnd_by_tunnel_ip.pop(tunnel_ip)

        self.pool[pool_name].free(tunnel_ip)

        self.mark_as_modified()

        return tunnel_ip

    def free_with_delay(self, instance, pool_name):
        assert ((instance.host.name, instance.port, pool_name) in self.allocated)

        tunnel_ip = self.allocated.pop((instance.host.name, instance.port, pool_name))
        # self.mtn_fqnd_by_tunnel_ip.pop(tunnel_ip)

        expired_at = time.mktime((datetime.datetime.now() + datetime.timedelta(days=7)).timetuple())
        self.waiting_removal.append(dict(tunnel_ip=tunnel_ip, expired_at=expired_at))

        self.mark_as_modified()

        return tunnel_ip

    def ip_in_mask_list(self, ip):
        for ipv4pool in self.pool.itervalues():
            if ipv4pool.ip_in_mask_list(ip):
                return True
        return False

    def get_pool_by_ip(self, ip):
        for ipv4pool in self.pool.itervalues():
            if ipv4pool.ip_in_mask_list(ip):
                return ipv4pool

        raise Exception('Ip <{}> is not in our pools'.format(ip))

    def has(self, instance, pool_name):
        return (instance.host.name, instance.port, pool_name) in self.allocated

    def get(self, instance, pool_name):
        return self.allocated.get((instance.host.name, instance.port, pool_name), None)

    def get_host_by_ip(self, ip):
        """Get host by ip from allocated instances"""
        for (hostname, port, pool_name), candidate_ip in self.allocated.iteritems():
            if candidate_ip == ip:
                return hostname
        else:
            raise Exception('Ip <{}> not found in allocated instances')

    def get_mtn_host_by_ip(self, ip):
        """Get mtn host name from allocated instances"""
        for (hostname, port, pool_name), candidate_ip in self.allocated.iteritems():
            if candidate_ip == ip:
                host = self.db.hosts.get_host_by_name(str(hostname))
                instance = [x for x in self.db.groups.get_host_instances(host) if x.port == port][0]
                return gaux.aux_hbf.generate_mtn_hostname(instance, self.db.groups.get_group(instance.type), '')
        else:
            raise Exception('Ip <{}> not found in allocated instances')

    def get_instance_hbf_info_by_ip(self, ip):
        for (hostname, port, pool_name), candidate_ip in self.allocated.iteritems():
            if candidate_ip == ip:
                host = self.db.hosts.get_host_by_name(str(hostname))
                instance = [x for x in self.db.groups.get_host_instances(host) if x.port == port][0]
                return gaux.aux_hbf.generate_hbf_info(self.db.groups.get_group(instance.type), instance)
        else:
            raise Exception('Ip <{}> not found in allocated instances')

    def get_all_subnets(self):
        result = []
        for ipv4pool in self.pool.itervalues():
            result.extend(ipv4pool.loc_pools)
        return result

    def update(self, smart=False):
        if self.db.version < '2.2.37':
            return

        import gaux.aux_hbf

        # allocate ipv4 tunnells for groups with corresponding flag set
        result_keys = set()
        for group in self.db.groups.get_groups():
            if group.card.properties.ipip6_ext_tunnel == False and group.card.properties.ipip6_ext_tunnel_v2 == False:
                continue

            pool_name = group.card.properties.ipip6_ext_tunnel_pool_name

            for instance in group.get_kinda_busy_instances():
                result_keys.add((instance.host.name, instance.port, pool_name))
                if not self.has(instance, pool_name):
                    self.alloc(instance, pool_name)
        for k in self.allocated.keys():
            if k not in result_keys:
                hostname, port, pool_name = k
                tunnel_ip = self.allocated.pop(k)
                self.waiting_removal.append(dict(tunnel_ip=tunnel_ip, expired_at=int(time.time() + 7 * 24 * 60 * 60)))

        # write result
        result = {}
        if self.db.version <= '2.2.46':
            result['pool'] = self.pool['default'].to_json()
            result['allocated'] = []
            for host, port, pool_name in sorted(self.allocated.keys()):
                result['allocated'].append(dict(host=host, port=port, tunnel_ip=self.allocated[(host, port, pool_name)]))
        else:
            result['pool'] = {x: y.to_json() for x, y in self.pool.iteritems()}
            result['allocated'] = []
            for host, port, pool_name in sorted(self.allocated.keys()):
                result['allocated'].append(dict(host=host, port=port, pool_name=pool_name, tunnel_ip=self.allocated[(host, port, pool_name)]))
            # ================================================ RX-526 START =================================================
            result['waiting_removal'] = []
            for elem in self.waiting_removal:
                expired_at = time.strftime('%Y-%m-%d %H:%M', time.localtime(elem['expired_at']))
                result['waiting_removal'].append(dict(tunnel_ip=elem['tunnel_ip'], expired_at=expired_at))
            # ================================================ RX-526 FINISH ================================================


        with open(self.DATA_FILE, 'w') as f:
            json.dump(result, f, indent=4, sort_keys=True)

    def as_str_extended(self):
        by_pool_instances = defaultdict(lambda: defaultdict(list))
        for (hostname, port, pool_name), addr in self.allocated.iteritems():
            subnet_descr = self.pool[pool_name].get_subnet_descr(addr)
            by_pool_by_group_instances = by_pool_instances[subnet_descr]

            if not self.db.hosts.has_host(str(hostname)):
                by_pool_by_group_instances['UNKNOWN'].append((hostname, port))
                continue

            host = self.db.hosts.get_host_by_name(str(hostname))
            candidate_instances = self.db.groups.get_host_instances(host)
            candidate_instances = [x for x in candidate_instances if x.port == port]
            if not candidate_instances:
                by_pool_by_group_instances['UNKNOWN'].append((hostname, port))
                continue

            candidate_instance = candidate_instances[0]
            by_pool_by_group_instances[candidate_instance.type].append((hostname, port))

        result = []
        for subnet_descr in sorted(by_pool_instances):
            result.append('Subnet <{}>:'.format(subnet_descr))
            by_group_instances = by_pool_instances[subnet_descr]
            by_group_instances.items()
            for groupname, instances in sorted(by_group_instances.items(), key=lambda (x, y): -len(y)):
                instances_as_str = ' '.join('{}:{}'.format(x, y) for x, y in instances)
                result.append('    Group {} has {} instances: {}'.format(groupname, len(instances), instances_as_str))
        result = gaux.aux_utils.indent('\n'.join(result))
        return result


    def __str__(self):
        free_str = '    Free:'
        for pool_name in sorted(self.pool):
            free_str += '\n        Pool {}:\n'.format(pool_name)
            free_str += gaux.aux_utils.indent(self.pool[pool_name].format_stat(self.waiting_removal), ind=' '*12)

        return (
                'Ipv4 pool:\n'
                '    Allocated: {allocated}\n'
                '    WaitingRemoval: {waiting_removal}\n'
                '{free}'
               ).format(allocated=len(self.allocated), waiting_removal=len(self.waiting_removal), free=free_str)

    def fast_check(self, timeout):
        pass

    def rename_host(self, old_hostname, new_hostname):
        allocated = {}
        for (hostname, port, pool), hostdata in self.allocated.iteritems():
            if hostname == old_hostname:
                allocated[(new_hostname, port, pool)] = hostdata
                self.mark_as_modified()
            else:
                allocated[(hostname, port, pool)] = hostdata
        self.allocated = allocated

    def switch_hostname(self, left_hostname, right_hostname):
        def get_host_record(allocated, hostname):
            for key in allocated:
                if hostname == key[0]:
                    return hostname, allocated[key]
            return None, None

        l_hostname, l_host_data = get_host_record(self.allocated, left_hostname)
        r_hostname, r_host_data = get_host_record(self.allocated, right_hostname)

        if l_hostname is None or r_hostname is None:
            return

        for (hostname, port, pool), hostdata in self.allocated.iteritems():
            if hostname == l_hostname:
                self.allocated[(hostname, port, pool)] = r_host_data
            elif hostname == r_hostname:
                self.allocated[(hostname, port, pool)] = l_host_data
        self.mark_as_modified()
