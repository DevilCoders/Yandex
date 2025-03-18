#!/skynet/python/bin/python
"""
    This script construct reply to searcherlookup/groups/<groupname>/instances api request
"""

import hashlib
import gc
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
import ujson
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
import gaux.aux_volumes
import gaux.aux_decorators
import gaux.aux_portovm
from gaux.aux_hbf import generate_hbf_info, generate_mtn_hostname, generate_mtn_addr
from optimizers.dynamic.aux_dynamic import calc_instance_memory
import gaux.gpu as gpu


@gaux.aux_decorators.memoize
def detect_ipv6addr(hostname):
    if CURDB.dnscache.has(hostname):
        return CURDB.dnscache.get(hostname)
    else:
        return None


class Dummy(object):
    __slots__ = ['params']

    def __init__(self, params):
        self.params = params


def get_parser():
    parser = ArgumentParserExt(description="Generated searcherlookup for group in nanny format")
    parser.add_argument("-g", "--groups", type=core.argparse.types.comma_list, required=True,
                        help="Obligatory. List of groups to generate searcherlookup for")
    parser.add_argument("--silent", action="store_true", default=False,
                        help="Optional. Run without dumping result to stdout (check purposes only)")
    parser.add_argument("--limit-instances", type=int, default=None,
                        help="Optional. Limit number of intances per group in report (for debug printing)")
    parser.add_argument("--db", type=core.argparse.types.gencfg_db, default=CURDB,
                        help="Optional. Path to db")

    return parser


def normalize(_):
    pass


def generate_cpu_limits(group, instance):
    guarantee = instance.power / instance.host.power

    # group classes as descibed in
    # https://wiki.yandex-team.ru/JandeksPoisk/runtimeexpertise/gencfg/optimization/
    if group.card.properties.fake_group:  # fake group
        limit = instance.power / instance.host.power
    elif group.card.audit.cpu.class_name == 'psi':
        limit = 1
    elif group.card.audit.cpu.class_name in ('normal', 'testing'):
        limit = max(instance.power, 60) / instance.host.power
    elif group.card.audit.cpu.class_name == "greedy":
        if hasattr(group.card.audit.cpu, 'greedy_limit') and group.card.audit.cpu.greedy_limit is not None:
            limit = group.card.audit.cpu.greedy_limit / instance.host.power
            limit = min(1., limit)
            limit = max(guarantee, limit)
        else:
            limit = 1.
    else:
        raise Exception('Unsupported class <{}>'.format(group.card.audit.cpu.class_name))

    # ============================================= RX-291 START ===============================================
    if guarantee < 0.1 / instance.host.ncpu:
        guarantee = 0
    # ============================================= RX-291 FINISH ==============================================

    return {
        'cpu_guarantee': guarantee * 100,
        'cpu_limit': limit * 100,
        'cpu_cores_guarantee': guarantee * instance.host.ncpu,
        'cpu_cores_limit': limit * instance.host.ncpu,
    }


def generate_io_limits(group_instance_reqs):
    io_limits = {
        'hdd': {
            'bps': {'write': 0, 'read': 0},
            'ops': {'write': 0, 'read': 0}
        },
        'ssd': {
            'bps': {'write': 0, 'read': 0},
            'ops': {'write': 0, 'read': 0}
        }
    }
    for disk_type in ('hdd', 'ssd'):
        for access_type in ('read', 'write'):
            for io_limit_type, type_key in (('io', 'bps'), ('io_ops', 'ops')):
                field_name = '_'.join([disk_type, io_limit_type, access_type, 'limit'])
                if hasattr(group_instance_reqs, field_name):
                    io_limit = getattr(group_instance_reqs, field_name)
                    io_limits[disk_type][type_key][access_type] = int(io_limit
                                                                      if io_limit_type == 'io_ops'
                                                                      else io_limit.value)
    return io_limits


def generate_porto_limits(group, instance, is_guest_group=False):
    if (group.card.name, instance.power, instance.host.power, is_guest_group) not in generate_porto_limits.cache:
        group_instance_reqs = group.card.reqs.instances

        # zero limits (no limits) for group with memory_guarantee not set
        if group_instance_reqs.memory_guarantee.value == 0:
            memory_guarantee = 0
            memory_limit = 0
        else:
            if is_guest_group:
                memory_guarantee = int(group_instance_reqs.memory_guarantee.value)
            else:
                memory_guarantee = calc_instance_memory(group.parent.db, instance)
            memory_limit = memory_guarantee + int(group_instance_reqs.memory_overcommit.value)

        if 'net_limit' in group_instance_reqs:
            net_limit = int(group_instance_reqs.net_limit.value)
            net_guarantee = int(group_instance_reqs.net_guarantee.value)
        else:
            net_limit = 0
            net_guarantee = 0

        result = {
            'memory_guarantee': memory_guarantee,
            'memory_limit': memory_limit,
            'net_guarantee': net_guarantee,
            'net_limit': net_limit,
            'io_limits': generate_io_limits(group_instance_reqs),
        }
        result.update(generate_cpu_limits(group, instance))

        generate_porto_limits.cache[(group.card.name, instance.power, instance.host.power, is_guest_group)] = result
    return generate_porto_limits.cache[(group.card.name, instance.power, instance.host.power, is_guest_group)]


generate_porto_limits.cache = dict()


def generate_storages_info(group, _, guest_instance=False):
    if guest_instance:
        return {
            'rootfs': {
                'partition': 'ssd',
                'size': 0.0,
            },
        }

    if group.card.name not in generate_storages_info.cache:
        result = {}
        if group.card.reqs.instances.disk.value > 0:
            result['rootfs'] = {
                'partition': 'hdd',
                'size': group.card.reqs.instances.disk.gigabytes(),
            }
            if group.card.reqs.instances.ssd.value > 0:
                result['ssd'] = {
                    'partition': 'ssd',
                    'size': group.card.reqs.instances.ssd.gigabytes(),
                }
        else:
            result['rootfs'] = {
                'partition': 'ssd',
                'size': group.card.reqs.instances.ssd.gigabytes(),
            }

        generate_storages_info.cache[group.card.name] = result
    return generate_storages_info.cache[group.card.name]


generate_storages_info.cache = dict()


def generate_volume_info(group, instance, volume_info, shared_volume=False, guest_instance=False):
    """Generate volume info for instance in group"""

    m = hashlib.md5()
    if guest_instance:
        m.update('%s%s%s_GUEST%s' % (volume_info.host_mp_root, volume_info.guest_mp,
                                     group.card.name, instance.host.name))
    else:
        m.update('%s%s%s%s' % (volume_info.host_mp_root, volume_info.guest_mp, group.card.name, instance.host.name))

    # some difficulties if generating shared volumes (here we must find corresponding port of master group)
    if shared_volume:
        master_instances = group.get_host_instances(instance.host)
        master_instance = next((x for x in master_instances if x.N == instance.N), None)
        if not master_instance:
            master_instance = next((x for x in master_instances if x.N == 0), None)
        m.update(str(master_instance.port))
    else:
        m.update(str(instance.port))

    # =================================== RX-264 START ==============================================
    if volume_info.uuid_generator_version == 0:  # oldest variant
        pass
    elif volume_info.uuid_generator_version == 1:
        m.update(str(volume_info.generate_deprecated_uuid))
    elif volume_info.uuid_generator_version == 2:
        m.update(str(volume_info.quota.value))
    else:
        raise Exception('Unsupported version <{}>'.format(volume_info.uuid_generator_version))
    # =================================== RX-264 STOP ===============================================

    uuid = m.hexdigest()[:12]

    return {
        'dom0_mount_point_root': volume_info.host_mp_root,
        'quota': int(volume_info.quota.value),
        'guest_mount_point': volume_info.guest_mp,
        'uuid': uuid,
        'symlinks': volume_info.symlinks,
        'mount_point_workdir': volume_info.mount_point_workdir,
    }


def generate_volumes_info(group, instance, guest_instance=False):
    """GENCFG-811"""

    if guest_instance:
        group_volumes = list(gaux.aux_volumes.default_generic_volumes(group))
    else:
        if group.card.name not in generate_volumes_info.volumes_cache:
            generate_volumes_info.volumes_cache[group.card.name] = gaux.aux_volumes.volumes_as_objects(group)
        group_volumes = generate_volumes_info.volumes_cache[group.card.name]

    result = [generate_volume_info(group, instance, x, guest_instance=guest_instance) for x in group_volumes]

    # =============================== RX-76 START ====================================
    if group.card.master is not None:
        if group.card.master.card.name not in generate_volumes_info.volumes_cache:
            generate_volumes_info.volumes_cache[group.card.master.card.name] = gaux.aux_volumes.volumes_as_objects(group.card.master)
        master_group_volumes = generate_volumes_info.volumes_cache[group.card.master.card.name]
        for volume_info in master_group_volumes:
            if volume_info.shared:
                result.append(generate_volume_info(group.card.master, instance, volume_info, shared_volume=True))
    # =============================== RX-76 FINISH ===================================

    return result


generate_volumes_info.volumes_cache = dict()


def generate_monitoring_info(group, instance):
    """GENCFG-919"""

    if (group.card.name, instance.port) not in generate_monitoring_info.cache:
        result = {}
        if hasattr(group.card.properties, 'monitoring_ports_ready') and group.card.properties.monitoring_ports_ready:
            # RX-86: set juggler ports
            if hasattr(group.card.properties, 'monitoring_juggler_port') and (group.card.properties.monitoring_juggler_port is not None):
                result['juggler'] = dict(agent=dict(port=group.card.properties.monitoring_juggler_port))
            else:
                result['juggler'] = dict(agent=dict(port=instance.port + 7))

            if hasattr(group.card.properties, 'monitoring_golovan_port') and (group.card.properties.monitoring_golovan_port is not None):
                result['yasm'] = dict(agent=dict(port=group.card.properties.monitoring_golovan_port))
            else:
                result['yasm'] = dict(agent=dict(port=instance.port + 6))
        generate_monitoring_info.cache[(group.card.name, instance.port)] = result
    return generate_monitoring_info.cache[(group.card.name, instance.port)]


generate_monitoring_info.cache = dict()


def generate_iss_info(group, instance):
    """GENCFG-1103"""

    if hasattr(group.card.properties, 'allow_fast_download_queue') and group.card.properties.allow_fast_download_queue:
        allow_fast_download_queue = True
    else:
        allow_fast_download_queue = False

    return {
        'allow_fast_download_queue': allow_fast_download_queue,
    }


# ============================================= RX-301 START ===============================================
def generate_secondary_ports(group, instance):
    if (group.card.name, instance.port) not in generate_secondary_ports.cache:
        if group.card.legacy.funcs.instancePort.startswith('new'):
            result = range(instance.port + 1, instance.port + 8)
        else:
            result = []
        generate_secondary_ports.cache[(group.card.name, instance.port)] = result
    return generate_secondary_ports.cache[(group.card.name, instance.port)]


generate_secondary_ports.cache = dict()
# ============================================ RX-301 FINISH ===============================================


def ipv4addr(instance, is_guest_group):
    if is_guest_group:
        return None
    else:
        return instance.host.ipv4addr if instance.host.ipv4addr != 'unknown' else None


def ipv6addr(instance, is_guest_group):
    if is_guest_group:
        return detect_ipv6addr(instance.host.name)
    else:
        return instance.host.ipv6addr if instance.host.ipv6addr != 'unknown' else None


def generate_for_group(group_name, options):
    # ========================================== GENCFG-1654 START =====================================================
    if group_name == 'ALL_SEARCH_VM':
        is_guest_group = True
        instances = []
        for group in CURDB.groups.get_groups():
            if group.card.properties.nonsearch:
                continue
            if group.card.on_update_trigger is not None:
                continue
            if group.card.tags.itype not in ('psi', 'portovm'):
                continue
            instances.extend([gaux.aux_portovm.guest_instance(x, db=CURDB) for x in group.get_kinda_busy_instances()])

        for instance in instances:
            instance.port = 60160
    # ========================================== GENCFG-1654 FINISH ====================================================
    # ========================================== GENCFG-1852 START =====================================================
    elif group_name == 'ALL_SAMOGON_RUNTIME_VM':
        is_guest_group = True
        instances = []
        for group in CURDB.groups.get_groups():
            if group.card.properties.nonsearch:
                continue
            if group.card.on_update_trigger is not None:
                continue
            if 'samogon' not in group.card.tags.prj:
                continue

            guest_instances = []
            for instance in group.get_kinda_busy_instances():
                guest_instance = gaux.aux_portovm.guest_instance(instance, db=CURDB)
                guest_instance.host.name = generate_mtn_hostname(instance, group, '',
                                                                 generate_mtn_addr(instance, group, 'vlan688'))
                guest_instances.append(guest_instance)
            instances.extend(guest_instances)

        for instance in instances:
            instance.port = 60150
    # ========================================== GENCFG-1852 FINISH ====================================================
    elif group_name.endswith('_GUEST'):
        dom_group_name = group_name.rpartition('_')[0]
        if not options.db.groups.has_group(dom_group_name):
            return {'instances': []}

        is_guest_group = True
        group = options.db.groups.get_group(dom_group_name)
        group_searcherlookup = group.generate_searcherlookup(use_cached=False, normalize=False, add_guest_hosts=True)
        instances = group_searcherlookup.itags_auto['{}_GUEST'.format(group.card.name)]
    else:
        is_guest_group = False
        group = options.db.groups.get_group(group_name)
        group_searcherlookup = group.generate_searcherlookup(normalize=False)
        instances = group_searcherlookup.itags_auto[group.card.name]

    instances.sort()
    if options.limit_instances is not None:
        instances = instances[:options.limit_instances]

    if group_name == 'ALL_SEARCH_VM':
        instance_to_shard = {x: 'none' for x in instances}
        all_tags = lambda x: ['ALL_SEARCH_VM']
    elif group_name == 'ALL_SAMOGON_RUNTIME_VM':
        instance_to_shard = {x: 'none' for x in instances}
        all_tags = lambda x: ['ALL_SAMOGON_RUNTIME_VM']
    else:
        # FIXME: too slow
        instance_to_shard = dict()
        for (shard, tier), lst in group_searcherlookup.ilookup.iteritems():
            for instance in lst:
                instance_to_shard[instance] = shard
        all_tags = lambda x: group_searcherlookup.instances[x]

    result = []
    for instance in instances:
        result.append({
            'hostname': instance.host.name,
            'port': instance.port,
            'secondary_ports': generate_secondary_ports(group, instance),
            'security_segment': group.card.properties.security_segment,
            'tags': all_tags(instance),
            'power': max(instance.power, 1.0),
            'host_resources': {
                'disk': instance.host.disk,
                'memory': instance.host.memory,
                'model': instance.host.model,
                'n_disks': instance.host.n_disks,
                'ncpu': instance.host.ncpu,
                'net': instance.host.net,
                'ssd': instance.host.ssd,
            },
            'dc': instance.host.dc,
            'queue': instance.host.queue,
            'rack': instance.host.rack,
            'location': instance.host.location,
            'domain': instance.host.domain,
            'ipv4addr': ipv4addr(instance, is_guest_group),
            'ipv6addr': ipv6addr(instance, is_guest_group),
            'porto_limits': generate_porto_limits(group, instance, is_guest_group),
            'shard_name': instance_to_shard[instance],
            'storages': generate_storages_info(group, instances, guest_instance=is_guest_group),
            'volumes': generate_volumes_info(group, instance, guest_instance=is_guest_group),
            'hbf': generate_hbf_info(group, instance) if (not is_guest_group) else {},
            'monitoring': generate_monitoring_info(group, instance),
            'iss': generate_iss_info(group, instance),
        })
        host_devices = gpu.host_devices(group, instance)
        if host_devices:
            result[-1]['host_devices'] = host_devices

    result.sort(key=lambda i: (i['hostname'], i['port']))
    return {'instances': result}


def main(options):
    result = {}

    for group_name in options.groups:
        try:
            gc.disable()
            result[group_name] = generate_for_group(group_name, options)
        finally:
            gc.enable()

    return result


def print_result(result, options):
    if not options.silent:
        print ujson.dumps(result, indent=4, sort_keys=True)


def cli_main():
    options = get_parser().parse_cmd()
    normalize(options)
    print_result(main(options), options)


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


if __name__ == '__main__':
    cli_main()
