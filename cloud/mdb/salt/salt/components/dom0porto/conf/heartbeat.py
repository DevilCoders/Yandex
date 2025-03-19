#!/usr/bin/env python
"""
MDB dom0 heartbeat sender

https://wiki.yandex-team.ru/mdb/internal/operations/duty-howto/handling-alerts/dbmheartbeat/
"""

import argparse
import json
import os
import re
import socket
import subprocess

import requests

STATE_PATH = '/tmp/dom0_heartbeat.state'
DBM_TOKEN = '{{ salt["pillar.get"]("data:config:dbm_token", "") }}'
DBM_URL = '{{ salt["pillar.get"]("data:config:dbm_url", "") }}'
IGNORE_DISK_FILES = ['lost+found', '.ignore_mounts', 'quota.project']
KERNELS_WHITELIST = ['3.18.43-', '4.4.52-', '4.4.68-', '4.4.88-',
                     '4.19.183-42.1mofed',
                     '4.4.114-', '4.4.171-', '4.9.51-', '4.9.101-',
                     '4.14.94-', '4.19.44-', '4.19.55-', '4.19.79-',
                     '4.19.100-', '4.19.114-', '4.19.119-', '4.19.143-']
DEFAULT_NET_SPEED = 1024 * 1024 * 1024 // 8
DEFAULT_SSD_DISK_SPEED = 250
# Measure speed https://wiki.yandex-team.ru/mdb/internal/operations/duty-howto/handling-alerts/dbmheartbeat/#missingspeedformodel
SSD_DISKS_SPEED = {
    'SAMSUNGMZWLR7T6HALA-00007': 5520,
    'SDLF1CRR-019T-1JA2': 250,  # Measured: 68.9
    'SDLF1DAR-960G-1HA1': 250,  # Measured: 81.3
    'SDLF1CRR-019T-1HA1': 250,  # Measured: 81.4
    'SDLF1CRR-019T-1JA1': 250,  # Measured: 86.8
    'TOSHIBATHNSNH512GCST': 250,  # Measured: 101
    'SAMSUNGMZ7WD960HMHP-00003': 250,  # Measured: 143
    'INTELSSDSA2BW600G3': 250,  # Measured: 146
    'SanDiskSD7SB7S960G': 250,  # Measured: 147
    'INTELSSDSC2CW480A3': 250,  # Measured: 159
    'KINGSTONSE50S3480G': 250,  # Measured: 159
    'TOSHIBATHNSNJ960PCSZ': 250,  # Measured: 160
    'MICRON_M510DC_MTFDDAK960MBP': 250,  # Measured: 166
    'TOSHIBATHNSN8960PCSE': 250,  # Measured: 168
    'INTELSSDSC2BW480A3': 250,  # Measured: 170
    'INTELSSDSA2CW600G3': 250,  # Measured: 171
    'INTELSSDSA2CW300G3': 250,  # Measured: 172
    'SAMSUNGMZ7KM960HAHP-00005': 250,  # Measured: 174
    'SAMSUNGMZ7GE900HMHP-000DX': 250,  # Measured: 178
    'INTELSSDSC2BB600G4': 250,  # Measured: 180
    'Micron_5100_MTFDDAK960TCB': 250,  # Measured: 182
    'INTELSSDSC2BB300G4': 250,  # Measured: 183
    'INTELSSDSA2BW300G3': 250,  # Measured: 187
    'Micron_M500DC_MTFDDAK480MBB': 250,  # Measured: 189
    'SAMSUNGMZ7PD480HAGM-000DA': 250,  # Measured: 193
    'SAMSUNGMZ7LM960HMJP-00005': 250,  # Measured: 194
    'INTELSSDSC2BB480G4': 250,  # Measured: 196
    'SanDiskSD7SB7S010T1122': 250,  # Measured: 197
    'XF1230-1A0960': 250,  # Measured: 198
    'SAMSUNGMZ7KH960HAJR-00005': 250,  # Measured: 199
    'SAMSUNGMZ7LH960HAJR-00005': 380,
    'SAMSUNGMZ7KM480HAHP-00005': 250,  # Measured: 201
    'SAMSUNGMZ7WD480HMHP-00003': 250,  # Measured: 203
    'INTELSSDSC2BW480A4': 250,  # Measured: 215
    'INTELSSDSC2BB800G4': 250,  # Measured: 216
    'INTELSSDSC2BB800G7': 250,  # Measured: 218
    'Micron_M500DC_MTFDDAK800MBB': 250,  # Measured: 225
    'ST480FP0021': 270,
    'Micron_5100_MTFDDAK1T9TCB': 280,
    'Micron_5200_MTFDDAK960TDD': 300,
    'INTELSSDSC2BB800G6': 300,
    'Micron_5100_MTFDDAK960TCC': 310,
    'SAMSUNGMZ7LM960HCHP-00003': 310,
    'TOSHIBATHNSN81Q92CSE': 310,
    'Micron_5200_MTFDDAK960TDC': 320,
    'TOSHIBAKHK61RSE960G': 320,
    'Micron_5100_MTFDDAK960TBY': 330,
    'Micron_5200_MTFDDAK1T9TDC': 330,
    'Micron_5100_MTFDDAK1T9TBY': 340,
    'XA1920ME10063': 350,
    'INTELSSDSC2KG960G7': 350,
    'Micron_5300_MTFDDAK960TDS': 360,
    'INTELSSDSC2KG019T8': 370,
    'Micron_5200_MTFDDAK1T9TDD': 370,
    'Micron_M600_MTFDDAK1T0MBF': 370,
    'XA3840ME10063': 370,
    'SAMSUNGMZ7KH1T9HAJR-00005': 380,
    'SAMSUNGMZWLL3T2HAJQ-00005': 2580,
    'Micron_9200_MTFDHAL3T2TCU': 2840,
    'Micron_9300_MTFDHAL3T2TDR': 3200,
    'SAMSUNGMZWLJ7T6HALA-00007': 3210,
    'XA960ME10063': 360,
    'SAMSUNGMZ7KH3T8HALS-00005': 380,
    'Micron_9300_MTFDHAL6T4TDR': 800,
    'Micron_5300_MTFDDAK1T9TDS': 330,
    'INTELSSDSC2KB019T8': 330,
    'TOSHIBATHNSNH512GBST': 250,  # Measured: 170
    'SAMSUNGMZWLR3T8HBLS-00007': 1740,
    'SAMSUNGMZWLJ3T8HBLS-00007': 1760,
    'INTELSSDPE2KE032T8': 820,
    'INTELSSDSC2KB960G8': 250,  # Measured: 202
}


def get_switch():
    """
    Get switch name from lldp
    """
    switch = None
    warnings = []
    try:
        proc = subprocess.Popen(['/usr/bin/timeout', '1', '/usr/sbin/lldpctl', '-f', 'keyvalue'],
                                stdout=subprocess.PIPE)
        output = proc.communicate()[0]
        for line in output.splitlines():
            if line.startswith('lldp.eth') and '.chassis.name=' in line:
                switch = line.split('=')[1].split('.')[0]
                break
    except Exception as exc:
        warnings.append('Unable to get lldp info: {error}'.format(error=repr(exc)))

    if switch is None:
        warnings.append('Unable to find switch with lldp')

    return switch, warnings


def get_project(fqdn):
    """
    Guess node project by fqdn
    """
    if '-pgaas-' in fqdn:
        return 'pgaas'
    if '-test-' in fqdn:
        return 'sandbox'
    if 'disk.yandex.net' in fqdn:
        return 'disk'
    return 'pers'


def get_memory():
    """
    Get memory info from /proc filesystem
    """
    with open('/proc/meminfo', 'r') as meminfo:
        for line in meminfo.readlines():
            if line.startswith('MemTotal:'):
                _, memory_kb, _ = line.split()

    # We round up to 1MB of RAM.
    memory = round(int(memory_kb) / 1024) * 1024 * 1024
    # Overcommit is forbidden, 3Gb are reserved for host system.
    memory -= 3 * (1024 * 1024 * 1024)
    # 1% is temporary reserved until MDB-5164 is resolved.
    memory -= round(0.01 * memory / 1024 / 1024) * 1024 * 1024
    return int(memory)


def get_slowest_disk(disks_info):
    """
    Get slowest disk in array
    """
    warnings = []
    slowest = None
    for disk_info in disks_info:
        for key, value in disk_info.items():
            if value['disk_type'] == 'hdd':
                return 25, warnings
            try:
                model = None
                proc = subprocess.Popen(
                    ['/usr/bin/timeout', '1', '/usr/sbin/smartctl', '-i', '/dev/{disk}'.format(disk=key)],
                    stdout=subprocess.PIPE)
                output = proc.communicate()[0]
                for line in output.splitlines():
                    if 'Model' in line and 'Family' not in line:
                        model = ''.join(line.split(':')[1].strip().split())
                        break
                if not model:
                    warnings.append('Unable to get model for {key}'.format(key=key))
                if model not in SSD_DISKS_SPEED:
                    speed = DEFAULT_SSD_DISK_SPEED
                    warnings.append('Missing speed for model {model} ({key})'.format(model=model, key=key))
                else:
                    speed = SSD_DISKS_SPEED[model]
                    if slowest is None:
                        slowest = speed
                    slowest = min(slowest, speed)
            except Exception as exc:
                warnings.append('Error on {key}: {error}'.format(key=key, error=repr(exc)))
    if slowest is None:
        warnings.append('Unable to get model for all disks')
        slowest = DEFAULT_SSD_DISK_SPEED
    return slowest, warnings


def get_disk_info():
    """
    Extract disk info from /etc/server_info.json
    """
    ssd_space = 0
    sata_space = 0
    max_io = 0

    with open('/etc/server_info.json', 'r') as server_info:
        info = json.load(server_info)
        for mountpoint in info['points_info']:
            slowest, warnings = get_slowest_disk(info['points_info'][mountpoint]['disks_info'])
            first_disk = info['points_info'][mountpoint]['disks_info'][0]
            for key in first_disk.keys():
                disk_type = first_disk[key]['disk_type']

            stat = os.statvfs(mountpoint)
            size = stat.f_bsize * stat.f_blocks

            num_disks = len(info['points_info'][mountpoint]['disks_info'])

            if disk_type == 'hdd':
                sata_space += size
            else:
                ssd_space += size
            max_io += slowest * num_disks

    if ssd_space > 0:
        ssd_space -= 10 * 1024 * 1024 * 1024
    if sata_space > 0:
        sata_space -= 10 * 1024 * 1024 * 1024
    max_io *= 1024 * 1024

    return ssd_space, sata_space, max_io, warnings


def get_disks():
    """
    Get raw disks (/disks/<uuid>)
    """
    ret = []

    if os.path.exists('/disks'):
        with open('/proc/mounts') as proc_mounts:
            mounts = [x.split() for x in proc_mounts]
        for disk_id in os.listdir('/disks'):
            full_path = os.path.join('/disks', disk_id)
            if not os.path.exists(os.path.join(full_path, '.ignore_mounts')):
                device = None
                for mount in mounts:
                    if mount[1] == full_path:
                        device = mount[0]
                        break
                if not device:
                    continue
            stat = os.statvfs(full_path)
            size = stat.f_bsize * stat.f_blocks
            has_data = bool([
                x for x in os.listdir(full_path) if x not in IGNORE_DISK_FILES
            ])
            ret.append({
                'id': disk_id,
                'max_space_limit': size,
                'has_data': has_data,
            })

    return ret


def get_net_speed():
    """
    Get network info from /proc/net/if_inet6 and /sys/class/net
    """
    if not os.path.exists('/proc/net/if_inet6'):
        return DEFAULT_NET_SPEED, ['Unable to find /proc/net/if_inet6']
    with open('/proc/net/if_inet6') as inp:
        if_inet6 = inp.read()

    selected_interface = None

    for line in if_inet6.splitlines():
        tokens = line.split()
        merged_addr = tokens[0]
        interface = tokens[-1]
        if 'eth' in interface and merged_addr.startswith('2'):
            selected_interface = interface
            break

    if selected_interface is None:
        return DEFAULT_NET_SPEED, ['Unable to find interface with ipv6 address']

    with open(os.path.join('/sys/class/net', selected_interface, 'speed')) as inp:
        speed = inp.read()

    net_speed = int(speed) * 1024 * 1024 // 8
    return net_speed, []


def heartbeat(fqdn, data):
    """
    Call DBM backend via REST API to report heartbeat
    """
    if not data.get('heartbeat'):
        return []
    try:
        url = '{url}/api/v2/dom0/{fqdn}'.format(url=DBM_URL, fqdn=fqdn)
        headers = {
            'Content-Type': 'application/json',
            'Authorization': 'OAuth {token}'.format(token=DBM_TOKEN),
        }
        res = requests.post(
            url, headers=headers, data=json.dumps(data), verify='/opt/yandex/allCAs.pem', timeout=5)
        if res.status_code != 200:
            raise Exception('Request to DBM failed ({code}: {text})'.format(
                code=res.status_code, text=res.text))
        return []
    except Exception as exc:
        return ['DBM interaction error: {error}'.format(error=repr(exc))]


def porto_alive(data):
    """
    Check if porto daemon is alive
    """
    try:
        import porto
        conn = porto.Connection()
        conn.connect()
        _ = conn.ListContainers()
        return []
    except Exception as exc:
        data['heartbeat'] = False
        return ['Porto check failed: {error}'.format(error=repr(exc))]


def kernel_check(data):
    """
    Check if booted kernel in whitelist
    """
    version = os.uname()[2]
    for whilelist_version in KERNELS_WHITELIST:
        if version.startswith(whilelist_version):
            return []

    data['heartbeat'] = False
    return ['Version {version} is not in whitelist: {whitelist}'.format(
        version=version, whitelist=', '.join(KERNELS_WHITELIST))]


def get_cpu_model(cpuinfo_file='/proc/cpuinfo'):
    """
    Get CPU model
    """
    try:
        with open(cpuinfo_file) as fd:
            for line in fd:
                match = re.match(r'model name\s+:\s*(.*)$', line)
                if match is not None:
                    return match.group(1), []
            return '', ["cpu model not found in '%s'" % cpuinfo_file]
    except IOError as exc:
        return '', ["Unable to get cpu model from '%s': %s" % (cpuinfo_file, exc)]


_INTEL_CPUS = {
    # Intel(R) Xeon(R) Gold 6338 CPU @ 2.00GHz
    r'Gold 63\d{2} CPU': 4,
}


def guess_generation(net_speed, num_cpus, cpu_model):
    """
    Compute dom0 generation
    """
    if cpu_model.startswith('Intel(R)'):
        for reg, generation in _INTEL_CPUS.items():
            if re.search(reg, cpu_model):
                return generation

    generation = 1
    if net_speed >= 1310720000:
        if num_cpus >= 80:
            generation = 3
        elif num_cpus >= 56:
            generation = 2
    return generation


def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--debug', action='store_true', help='Print result instead of sending it')
    args = parser.parse_args()

    fqdn = socket.gethostname()
    geo = fqdn[:3]
    num_cpus = os.sysconf('SC_NPROCESSORS_ONLN')
    ssd_space, sata_space, max_io, disk_warnings = get_disk_info()
    net_speed, net_warnings = get_net_speed()
    switch, switch_warnings = get_switch()
    cpu_model, cpu_warnings = get_cpu_model()

    generation = guess_generation(net_speed=net_speed, num_cpus=num_cpus, cpu_model=cpu_model)

    data = {
        'project': get_project(fqdn),
        'geo': geo,
        'cpu_cores': num_cpus,
        'memory': get_memory(),
        'ssd_space': ssd_space,
        'sata_space': sata_space,
        'max_io': max_io,
        'net_speed': net_speed,
        'generation': generation,
        'heartbeat': True,
        'disks': get_disks(),
        'switch': switch,
    }

    if os.path.exists('/etc/dbm_heartbeat_override.json'):
        with open('/etc/dbm_heartbeat_override.json') as override:
            data.update(json.load(override))

    warnings = disk_warnings + net_warnings + switch_warnings + cpu_warnings
    warnings += porto_alive(data)
    warnings += kernel_check(data)
    if args.debug:
        print(json.dumps(data, indent=4, sort_keys=True))
        print(json.dumps({'warnings': warnings}, indent=4, sort_keys=True))
    else:
        warnings += heartbeat(fqdn, data)
        tmp_path = STATE_PATH + '.tmp'
        with open(tmp_path, 'w') as out:
            json.dump({'warnings': warnings, 'heartbeat': data['heartbeat']}, out)
        os.rename(tmp_path, STATE_PATH)


if __name__ == '__main__':
    _main()
