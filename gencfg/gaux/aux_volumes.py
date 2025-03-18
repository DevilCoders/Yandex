"""Auxiliarily functions to work with volumes"""

import copy
import json

from core.card.node import CardNode
from core.card.types import ByteSize

from gaux.aux_utils import indent

GB = 1024 * 1024 * 1024
DEFAULT_UUID_GENERATOR_VERSION = 2  # GENCFG-1820


class TVolumeInfo(object):
    __slots__ = ('quota', 'host_mp_root', 'guest_mp', 'symlinks', 'shared', 'generate_deprecated_uuid', 'uuid_generator_version', 'mount_point_workdir')

    def __init__(self, quota=None, host_mp_root=None, guest_mp=None, symlinks=None, shared=False, generate_deprecated_uuid=False,
                 uuid_generator_version=DEFAULT_UUID_GENERATOR_VERSION, mount_point_workdir=False):
        self.quota = quota
        self.host_mp_root = host_mp_root
        self.guest_mp = guest_mp

        if symlinks is None:
            self.symlinks = []
        else:
            self.symlinks = symlinks

        self.shared = shared
        self.generate_deprecated_uuid = generate_deprecated_uuid
        self.uuid_generator_version = uuid_generator_version
        self.mount_point_workdir = mount_point_workdir

        if guest_mp == '':
            self.mount_point_workdir = True

    def __eq__(self, other):
        return self.quota == other.quota and self.host_mp_root == other.host_mp_root and \
            self.guest_mp == other.guest_mp and self.symlinks == other.symlinks and \
            self.shared == other.shared and self.generate_deprecated_uuid == other.generate_deprecated_uuid

    def __ne__(self, other):
        return not (self == other)

    def to_json(self):
        """Convert to json"""
        return dict(quota=self.quota.text, host_mp_root=self.host_mp_root, guest_mp=self.guest_mp,
                    symlinks=self.symlinks, shared=self.shared, generate_deprecated_uuid=self.generate_deprecated_uuid,
                    uuid_generator_version=self.uuid_generator_version, mount_point_workdir=self.mount_point_workdir)

    @classmethod
    def from_json(cls, jsoned):
        extra_keys = set(jsoned.keys()) - set(cls.__slots__)
        if extra_keys:
            raise Exception('Found extra keys <{}> in volumes json'.format(','.join(sorted(extra_keys))))

        if 'generate_deprecated_uuid' not in jsoned:
            jsoned['generate_deprecated_uuid'] = False
        if 'mount_point_workdir' not in jsoned:
            jsoned['mount_point_workdir'] = False
        if 'uuid_generator_version' not in jsoned:
            jsoned['uuid_generator_version'] = DEFAULT_UUID_GENERATOR_VERSION

        missing_keys = set(['quota', 'host_mp_root', 'guest_mp']) - set(jsoned.keys())
        if missing_keys:
            raise Exception('Missing keys <{}> in volumes json'.format(','.join(sorted(missing_keys))))

        return cls(quota=ByteSize(jsoned['quota']), host_mp_root=jsoned['host_mp_root'],
                   guest_mp=jsoned['guest_mp'], symlinks=jsoned.get('symlinks', None),
                   shared=jsoned.get('shared', False), generate_deprecated_uuid=jsoned.get('generate_deprecated_uuid', False),
                   uuid_generator_version=jsoned.get('uuid_generator_version', DEFAULT_UUID_GENERATOR_VERSION),
                   mount_point_workdir=jsoned.get('mount_point_workdir', False))

    def to_card_node(self):
        """Convert to card node"""
        node = CardNode()
        node.quota = self.quota
        node.host_mp_root = self.host_mp_root
        node.guest_mp = self.guest_mp
        node.symlinks = self.symlinks
        node.shared = self.shared
        node.generate_deprecated_uuid = self.generate_deprecated_uuid
        node.uuid_generator_version = self.uuid_generator_version
        node.mount_point_workdir = self.mount_point_workdir
        return node

    @classmethod
    def convert_value_from_str(cls, key, value):
        """Convert one of param from str"""
        def convert_bool(value):
            if value == 'True':
                return True
            if value == 'False':
                return False
            return ValueError('Can not covert <{}> to bool (must specify <True> or <False>)'.format(value))

        if key == 'quota':
            return ByteSize(value)
        elif key in ('host_mp_root', 'guest_mp'):
            return value
        elif key == 'symlinks':
            return value.split(',')
        elif key in ('shared', 'generate_deprecated_uuid', 'mount_point_workdir'):
            return convert_bool(value)
        elif key == 'uuid_generator_version':
            return int(value)
        else:
            raise Exception('Unknown key <{}>'.format(key))

    def __str__(self):
        result = ('host_mp_root=<{self.host_mp_root}> guest_mp=<{self.guest_mp}> quota=<{self.quota}> shared=<{self.shared}> '
                  'symlinks={self.symlinks} generate_deprecated_uid=<{self.generate_deprecated_uuid}> '
                  'mount_point_workdir=<{self.mount_point_workdir}>').format(self=self)
        return result


def default_generic_volumes(group):
    return [
        TVolumeInfo(quota=ByteSize("1.5 Gb"), host_mp_root='/place', guest_mp='/', symlinks=[], shared=False, generate_deprecated_uuid=False, mount_point_workdir=False),
        TVolumeInfo(quota=ByteSize("1 Gb"), host_mp_root='/place', guest_mp='', shared=False, generate_deprecated_uuid=False, mount_point_workdir=True),
        TVolumeInfo(quota=ByteSize("1 Gb"), host_mp_root='/place', guest_mp='/logs', symlinks=['/usr/local/www/logs'],
                    shared=False, generate_deprecated_uuid=False, mount_point_workdir=False),
    ]


def default_samogon_volumes(group):
    """Volumes for samogon differ from volumes for common groups (GENCFG-1956)"""

    GB = 1024 * 1024 * 1024

    result = [
        TVolumeInfo(quota=ByteSize("50 Gb"), host_mp_root='/place', guest_mp='/', symlinks=[], shared=False, generate_deprecated_uuid=False, mount_point_workdir=False),
        TVolumeInfo(quota=ByteSize("1 Gb"), host_mp_root='/place', guest_mp='', shared=False, generate_deprecated_uuid=False, mount_point_workdir=True),
        TVolumeInfo(quota=ByteSize("1 Gb"), host_mp_root='/place', guest_mp='/logs', symlinks=['/usr/local/www/logs'],
                    shared=False, generate_deprecated_uuid=False, mount_point_workdir=False),
    ]

    if group.card.reqs.instances.disk.value > 52 * GB:
        txt = '{:.2f} Gb'.format((group.card.reqs.instances.disk.value - 52 * GB) / float(GB))
        result.append(TVolumeInfo(quota=ByteSize(txt), host_mp_root='/place', guest_mp='/storage',
                                  shared=False, generate_deprecated_uuid=False, mount_point_workdir=False))

    if group.card.reqs.instances.ssd.value > 0:
        result.append(TVolumeInfo(quota=group.card.reqs.instances.ssd, host_mp_root='/ssd', guest_mp='/ssd',
                                  shared=False, generate_deprecated_uuid=False, mount_point_workdir=False))

    return result


def volumes_as_objects(group):
    if hasattr(group.card.reqs, 'volumes'):
        result = [TVolumeInfo(quota=x.quota, host_mp_root=x.host_mp_root, guest_mp=x.guest_mp,
                              symlinks=copy.copy(x.symlinks), shared=False if (not hasattr(x, 'shared')) else x.shared,
                              generate_deprecated_uuid=True if not hasattr(x, 'generate_deprecated_uuid') else x.generate_deprecated_uuid,
                              uuid_generator_version=x.uuid_generator_version if hasattr(x, 'uuid_generator_version') else DEFAULT_UUID_GENERATOR_VERSION,
                              mount_point_workdir=True if not hasattr(x, 'mount_point_workdir') else x.mount_point_workdir) for x in group.card.reqs.volumes]
    else:
        result = []

    if len(result) == 0:  # we use default volumes only if volumes not specified in group card
        if 'samogon' in group.card.tags.prj:
            result = default_samogon_volumes(group)
        elif group.card.properties.fake_group:
            result = []
        else:
            result = default_generic_volumes(group)

    return result


def volumes_as_json_string(group):
    """List of volumes as json string (RX-83)"""
    group_volumes = volumes_as_objects(group)
    group_volumes.sort(key=lambda x: x.guest_mp)
    group_volumes = [x.to_json() for x in group_volumes]

    for group_volume in group_volumes:
        group_volume.pop('uuid_generator_version')

    return json.dumps(group_volumes, indent=4, separators=(',', ': '), sort_keys=True)


def get_ssd_hdd_guarantee(group):
    """Get ssd and hdd guarantee from volumes"""

    volumes = volumes_as_objects(group)

    need_ssd, need_hdd = 0, 0

    for volume in volumes:
        if volume.host_mp_root == '/place':
            need_hdd += volume.quota.value
        elif volume.host_mp_root == '/ssd':
            need_ssd += volume.quota.value

    need_ssd_str = '{:.2f} Gb'.format(need_ssd / 1024. / 1024 / 1024)
    need_hdd_str = '{:.2f} Gb'.format(need_hdd / 1024. / 1024 / 1024)

    return need_ssd, need_ssd_str, need_hdd, need_hdd_str


def update_ssd_hdd_group_reqs(group):
    """Update ssd/hdd group reqs based on volumes information"""
    need_ssd, need_ssd_str, need_hdd, need_hdd_str = get_ssd_hdd_guarantee(group)

    need_recluster = False
    if need_ssd != group.card.reqs.instances.ssd.value:
        group.card.reqs.instances.ssd = ByteSize(need_ssd_str)
        need_recluster = True
        group.mark_as_modified()
    if need_hdd != group.card.reqs.instances.disk.value:
        group.card.reqs.instances.disk = ByteSize(need_hdd_str)
        need_recluster = True
        group.mark_as_modified()

    if group.card.master is not None and group.card.master.card.name == 'ALL_DYNAMIC':
        force_recluster = True
    else:
        force_recluster = False

    # ======================================== RX-548 START =======================================
    if need_recluster and force_recluster:
        from optimizers.dynamic.recluster import jsmain as dynamic_recluster
        params = dict(group=group, ssd=need_ssd_str, hdd=need_hdd_str, update=False)
        report = dynamic_recluster(params)
        print 'Had to recluster group {} after changing its volumes:'.format(group.card.name)
        print indent(report.report_text())
    # ======================================== RX-548 FINISH =======================================
