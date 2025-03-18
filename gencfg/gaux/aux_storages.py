"""Auxiliarily functions for detecting hardware storages configuration"""

import os
import re
import copy

from gaux.aux_utils import run_command


class TMpInfo(object):
    """
        Mount point info
    """
    def __init__(self, name, size, partitions, raid_mode="raid0"):
        self.name = name
        self.size = size
        self.partitions = partitions
        self.raid_mode = raid_mode
        self.rota = False
        self.disk_models = []

    def __str__(self):
        return "%s (mode %s, type %s): %d (partitions %s) (models %s)" % (
            self.name, self.raid_mode,
            "hdd" if self.rota else "ssd",
            self.size,
            " ".join(self.partitions),
            " ".join(self.disk_models))


def detect_mount_points():
    """
        Resolve mount point devices

        :return dict(str, str): dict of mount points to device names
    """

    result = []

    code, out, err = run_command(['df'])

    for line in out.strip().split('\n'):
        if not line.startswith('/'):
            continue
        parts = filter(lambda x: x != '', line.split(' '))

        device = parts[0]
        if not device.startswith('/'):
            continue
        if os.path.islink(device):
            device = os.path.realpath(device)
        if not device.startswith('/dev'):  # strange device files
            continue
        device = device[5:]
        if device in map(lambda x: x.partitions[0], result):  # some devices can be mounted on multiple filesystemp paths
            continue

        mpoint = parts[5]
        size = int(float(parts[1]) / 1024. / 1024)

        result.append(TMpInfo(mpoint, size, [device]))

    return result


class TRaidInfo(object):
    """
        Raid info
    """

    def __init__(self, name, raid_mode, partitions):
        self.name = name
        self.raid_mode = raid_mode
        self.partitions = partitions

    def __str__(self):
        return "%s (mode %s): %s" % (self.name, self.raid_mode, " ".join(self.partitions))


def detect_raids(mount_points):
    """
        Create mapping (<raid device>, <raid mode>) -> list of <disk partition> for all mount points
    """

    result = []

    mount_point_partitions = sum(map(lambda x: x.partitions, mount_points), [])

    code, out, err = run_command(['cat', '/proc/mdstat'])
    for line in out.split('\n'):
        line = line.strip()
        if line == '':
            continue

        raid_device = line.split(' ')[0]
        if raid_device in mount_point_partitions:
            raid_mode = line.split(' ')[3]
            partitions = map(lambda x: x.partition('[')[0], line.split(' ')[4:])
            result.append(TRaidInfo(raid_device, raid_mode, partitions))

    return result


class TPartitionInfo(object):
    """
        Partition info
    """

    def __init__(self, name, disk_name, size, rota):
        self.name = name
        self.disk_name = disk_name
        self.size = size
        self.rota = rota

    def __str__(self):
        return "%s: %d (%s %s)" % (self.name, self.size, self.disk_name, "hdd" if self.rota else "ssd")


def detect_partitions():
    """
        Detect list of partitions using lsblk command. I do not know how we can detect disk name correctly, so have to use
        link names from /dev/disk/by-id .
    """

    # find mapping partitions to disk names
    device_disk_names = dict()
    for fname in os.listdir('/dev/disk/by-id'):
        if not fname.startswith('ata-'):
            continue

        disk_model = fname.partition('-')[2].rpartition('_')[0]
        disk_model = disk_model.replace('_', ' ')

        real_device_path = os.path.realpath(os.path.join('/dev/disk/by-id', fname))
        real_device_name = real_device_path.rpartition('/')[2]

        device_disk_names[real_device_name] = disk_model

    def _to_dict(line):
        """
            Convert line from lsblk to dict
        """
        parts = re.findall('.*?=".*?"', line)
        parts = map(lambda x: x.lstrip().strip(), parts)
        result = dict(map(lambda x: (x.partition('=')[0], x.partition('=')[2].replace('"', '')), parts))
        return result

    result = []

    code, out, err = run_command(['lsblk', '-io', 'KNAME,TYPE,SIZE,MODEL,ROTA', '-b', '-P'])
    for line in out.strip().split('\n'):
        d = _to_dict(line)
        if d['TYPE'] == 'disk':
            last_disk = {'name': d['KNAME'], 'model': d['MODEL'].strip(), 'size': int(d['SIZE']), 'rota': int(d['ROTA'])}
        if d['TYPE'].startswith('part') or d['TYPE'] == 'disk':
            assert (last_disk is not None)
            if last_disk['model'] != '':
                result.append(TPartitionInfo(d['KNAME'], device_disk_names[d['KNAME']], int(int(d['SIZE']) / 1024. / 1024 / 1024), bool(int(d['ROTA']))))
            elif last_disk['name'].startswith('nvme'):
                result.append(TPartitionInfo(d['KNAME'], 'Generic NVME', int(int(d['SIZE']) / 1024. / 1024 / 1024), bool(int(d['ROTA']))))

    return result


def convert_mount_points_to_storages_info(mount_points):
    """
        Based on current mount points configuration we should guess, which partitions will be presented as <ssd,hdd,...> storage
    """

    def _mpoint_to_dict(mpoint):
        return {
            'mount_point': mpoint.name,
            'size': mpoint.size,
            'rota': mpoint.rota,
            'raid_mode': mpoint.raid_mode,
            'models': mpoint.disk_models,
        }

    def _check_primary_partitions(primary_mpoints, all_mpoints):
        """
            Check if partitionas which are called 'primary' occupy almost all disk (do not skip cases, where most of disk is in
            some other mount points
        """
        primary_size = sum(map(lambda x: x.size, primary_mpoints))
        total_size = sum(map(lambda x: x.size, all_mpoints))
        koeff = 1.1 if total_size > 1000 else 1.2  # old machines with small disk can have relatively big system partition
        if primary_size * koeff < total_size:
            raise Exception("To small primary mount points <%s> size <%d> (total <%d>)" % (",".join(map(lambda x: x.name, primary_mpoints)), primary_size, total_size))

    mount_points_by_name = dict(map(lambda x: (x.name, x), mount_points))

    # have </place> and </place/berkanavt> mount points (pretty rare case)
    if ('/place' in mount_points_by_name) and ('/place/berkanavt' in mount_points_by_name):
        main_mpoint = mount_points_by_name['/place']
        berkanavt_mpoint = mount_points_by_name['/place/berkanavt']

        _check_primary_partitions([main_mpoint, berkanavt_mpoint], mount_points)

        main_key = 'hdd' if main_mpoint.rota else 'ssd'
        return {
            main_key: _mpoint_to_dict(main_mpoint),
            'berkanavt': _mpoint_to_dict(berkanavt_mpoint)
        }

    # have both </place> and </ssd> mount points
    if ('/place' in mount_points_by_name) and ('/ssd' in mount_points_by_name):
        ssd_mpoint = mount_points_by_name['/ssd']
        hdd_mpoint = mount_points_by_name['/place']

        _check_primary_partitions([ssd_mpoint, hdd_mpoint], mount_points)

        # the most default case for ALL_SEARCH (almost every new host is of this configuration type)
        if (not ssd_mpoint.rota) and (hdd_mpoint.rota):
            return {
                'ssd': _mpoint_to_dict(ssd_mpoint),
                'hdd': _mpoint_to_dict(hdd_mpoint),
            }
        elif (ssd_mpoint.rota):
            raise Exception("Ssd mount point <%s> is rotational somehow" % ssd_mpoint.name)
        elif (not hdd_mpoint.rota):
            raise Exception("Hdd mount point <%s> is non-rotational (ssd) somehow" % hdd_mpoint.name)

    # have only one main mount point: </place> or </> (some search machines are configured with </place> as main parition, others - as </>
    for pname in ['/place', '/']:
        if pname in mount_points_by_name:
            mpoint = mount_points_by_name[pname]

            _check_primary_partitions([mpoint], mount_points)

            if mpoint.rota:
                return {'hdd': _mpoint_to_dict(mpoint), }
            else:
                return {'ssd': _mpoint_to_dict(mpoint), }

    raise Exception("Could not convert mount points configuration to storages list")


def detect_configuration(verbose_level):
    # detect mount points
    mount_points = detect_mount_points()
    if verbose_level > 0:
        print "Mount points:"
        for mount_point in mount_points:
            print "    %s" % str(mount_point)

    # detect raids
    raid_devices = detect_raids(mount_points)
    if verbose_level > 0:
        print "Raids:"
        for raid_device in raid_devices:
            print "    %s" % str(raid_device)
    raid_devices_by_name = dict(map(lambda x: (x.name, x), raid_devices))

    # detect partitions
    partitions = detect_partitions()
    if verbose_level > 0:
        print "Partitions:"
        for partition in partitions:
            print "    %s" % str(partition)
    partitions_by_name = dict(map(lambda x: (x.name, x), partitions))

    # convert mount points to list of real devices
    for mount_point in mount_points:
        assert len(mount_point.partitions) == 1, "Mount point <%s> has <%d> partitions: %s" % (mount_point.name, len(mount_point.partitions), ",".join(mount_point.partitions))
        probably_raid_device = mount_point.partitions[0]
        if probably_raid_device in raid_devices_by_name:
            mount_point.partitions = copy.copy(raid_devices_by_name[probably_raid_device].partitions)
            mount_point.raid_mode = raid_devices_by_name[probably_raid_device].raid_mode
    if verbose_level > 0:
        print "Fixed mount points:"
        for mount_point in mount_points:
            print "    %s" % str(mount_point)

    # add size and other info to mount points
    filtered_mount_points = []
    for mount_point in mount_points:
        not_found_partitions = filter(lambda x: x not in partitions_by_name, mount_point.partitions)
        if len(not_found_partitions) > 0:
            if verbose_level > 0:
                print "Not found partitions <%s> while processing mount point <%s>" % (",".join(not_found_partitions), mount_point.name)
            continue

        partitions_rota = set(map(lambda x: partitions_by_name[x].rota, mount_point.partitions))
        if partitions_rota == set([False]):  # there are mixed raid devices where ssd and hdd combined in single raid
            mount_point.rota = False
        else:
            mount_point.rota = True

        mount_point.disk_models = map(lambda x: partitions_by_name[x].disk_name, mount_point.partitions)

        filtered_mount_points.append(mount_point)
    mount_points = filtered_mount_points
    if verbose_level > 0:
        print "Mount points with everything filled:"
        for mount_point in mount_points:
            print "    %s" % str(mount_point)

    # now convert mount points to what will be stored into hosts_data
    result = convert_mount_points_to_storages_info(mount_points)
    if verbose_level > 0:
        print "Storages info:"
        for storage_id in result:
            print "    %s: %s" % (storage_id, result[storage_id])

    return result
