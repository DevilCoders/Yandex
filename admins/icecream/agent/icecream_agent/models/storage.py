""" Icecream agent storage models """
import json
import psutil
import subprocess
import logging
import subprocess
import re
import lvm
from icecream_agent.libs.common_helpers import gbs


class BaseStorage(object):
    """ Meta class for dom0 storage system """

    name = None
    storage_type = None
    size = None
    free = None
    volumes = None
    _log = None

    @staticmethod
    def disks():
        """
        Detect ssd/hdd/mixed machine disk type
        rotational 1 == hdd
        rotational 0 == ssd
        :return: ssd or hdd, str
        """
        _log = logging.getLogger()
        disks_types = {'{0}': 'ssd', '{1}': 'hdd', '{0, 1}': 'ssd',
                       '{1, 0}': 'ssd'}
        cmd = ['/bin/lsblk', '-Jdno', 'rota,type']
        _log.debug('Checking storage disks type')
        output = subprocess.check_output(cmd).decode('utf-8')
        _log.debug('Storage disks: %s', output)
        disks = json.loads(output)['blockdevices']
        disks_rota = set(int(x['rota']) for x in disks if x['type'] == 'disk')
        try:
            _log.debug('Disks type: %s', disks_types[str(disks_rota)])
            return disks_types[str(disks_rota)]
        except KeyError:
            _log.error('Error while checking disks type. '
                       'Known types: %s , got: %s',
                       disks_types, disks_rota)
            raise StorageInvalidDisks

    @staticmethod
    def container_volume_name(name):
        """
        Volume name by its container name
        :return: volume name, str
        """
        raise NotImplementedError

    @staticmethod
    def volume_container_name(name):
        """
        Container name by its volume name
        :return: container name, str
        """
        raise NotImplementedError

    def get_volume(self, name):
        """
        Get volume object by volume name
        :param name: volume name, str
        :return: volume object
        """
        raise NotImplementedError

    def to_dict(self):
        """
        Representation of storage object in dict
        :return: storage object, dict
        """
        result = {
            'name': self.name,
            'type': self.storage_type,
            'size': self.size,
            'free': self.free,
            'disks': self.disks(),
        }
        if self.volumes is not None:
            result['volume'] = [volume.to_dict() for volume in self.volumes]
            
        return result


class BaseVolume(object):
    """ Base class for container disk volume on storage system """

    name = None
    path = None
    _storage = None
    size = None
    container = None
    _log = None

    def resize(self, new_size):
        """
        Resize volume
        :param new_size: new volume size in bytes, int
        :return: size after resize in bytes, int
        """
        raise NotImplementedError

    def to_dict(self):
        """
        Volume representation in dict
        :return: volume object, dict
        """
        result = {
            'name': self.name,
            'path': self.path,
            'size': self.size,
            'container': self.container
        }
        self._log.debug('Dumped volume to dict: %s', result)
        return result


class RAIDStorage(BaseStorage):
    """ RAID stotage model"""

    storage_type = 'raid'

    def __init__(self, name):
        self.name = name
        self.size, self.free = self._disk_size()

    def _disk_size(self):
        partitions = psutil.disk_partitions()
        free = 0
        total = 0
        for partition in partitions:
            if partition.mountpoint != "/":
                df = subprocess.Popen(["df", "--output=file,target,size,avail",  partition.device],
                                      stdout=subprocess.PIPE)
                out = df.communicate()[0]
                device, mount, size, avail = out.split(b'\n')[1].split()
                total += int(size)
                free += int(avail)

        # convert kilobyte to byte
        return total * 1024, free * 1024




class LVMStorage(BaseStorage):
    """ LVM storage model """

    storage_type = 'lvm'

    def __init__(self, name):
        _raw = lvm.vgOpen(name, 'r')
        self.name = name
        self.size = _raw.getSize()
        self.free = _raw.getFreeSize()
        self.volumes = [
            LVMVolume(
                lv.getName(),
                lv.getSize(),
                self
            ) for lv in _raw.listLVs()
        ]
        _raw.close()
        self._log = logging.getLogger()
        self._log.debug('Initialized LVMStorage object')

    @staticmethod
    def container_volume_name(name):
        """ Get volume name by its container name """
        return 'containers_{}'.format(name.replace('-', '--'))

    @staticmethod
    def volume_container_name(name):
        """ Get container name by its volume name """
        return name.replace('containers_', '').replace('--', '-')

    def get_volume(self, name):
        """ Get volume object by name """
        self._log.debug('Searching volume with name: %s', name)
        for volume in self.volumes:
            if volume.name == name:
                self._log.info('Found volume %s: %s', name, volume.to_dict())
                return volume
        self._log.error('No volume: %s', name)
        raise StorageVolumeNotFoundError

    def reload(self):
        """ Reload storage info, reread it from device """
        self._log.debug('Reloading storage %s: %s', self.name, self.to_dict())
        self.__init__(self.name)
        self._log.info('Reloaded storage %s: %s', self.name, self.to_dict())


class LVMVolume(BaseVolume):
    """ Logic volume object model """

    def __init__(self, name, size, storage):
        self.name = name
        self.size = size
        self._storage = storage
        self.path = '/dev/{}/{}'.format(self._storage.name, self.name)
        self.container = self._storage.volume_container_name(self.name)
        self._log = logging.getLogger()
        self._log.debug('Initialized LVMVolume object')

    def reload(self):
        """ Reload volume info """
        self._log.debug('Reloading volume %s: %s', self.name, self.to_dict())
        self._storage.reload()
        new_data = self._storage.get_volume(self.name)
        self.__init__(new_data.name, new_data.size, self._storage)
        self._log.info('Reloaded volume %s: %s', self.name, self.to_dict())

    def resize(self, new_size):
        """ Resize volume """
        _steps = [
            ['/sbin/lvresize', '-L', '{}G'.format(gbs(new_size)), self.path],
            ['/sbin/resize2fs', self.path],
            ['tune2fs', '-m1', self.path]
        ]
        self._log.debug('Resizing volume %s, old size: %s, new size: %s',
                        self.name, self.size, new_size)
        self.reload()
        if gbs(self.size) == gbs(new_size):
            self._log.info('Volume %s: new == old size (%s==%s)',
                           self.name, new_size, self.size)
            return self.size
        elif gbs(self.size) > gbs(new_size):
            self._log.error('Volume %s: new < old size (%s<%s). Unprocessable',
                            self.name, new_size, self.size)
            raise NotImplementedError('cannot make smaller')
        elif gbs(new_size-self.size) > gbs(self._storage.free):
            self._log.error('Volume %s: not enough space on storage, '
                            'new > free size (%s>%s)',
                            self.name, new_size, self._storage.free)
            raise StorageNotEnoughSpaceError

        self._log.debug('Sanity checks passed')

        for step in _steps:
            self._log.debug('Executing step: %s', step)
            result = subprocess.check_output(step)
            self._log.debug('Step said: %s', result)

        self.reload()
        self._log.info('Volume %s resize finished, size: %s',
                       self.name, self.size)
        return self.size


class StorageNotEnoughSpaceError(Exception):
    """ No space on storage """
    pass


class StorageNotFoundError(Exception):
    """ No such storage """
    pass


class StorageVolumeNotFoundError(Exception):
    """ No such volume """
    pass


class StorageInvalidDisks(Exception):
    """ Not ssd / hdd disks type """
    pass


def detect_storage():
    """ Detect lvm. IF VG 'lxd' doest not exist, must be a raid machine"""

    _log = logging.getLogger()
    vgroup = lvm.listVgNames()
    if "lxd" in vgroup:
        _log.info('LVM group detected')
        return LVMStorage("lxd")
    return RAIDStorage("raid")


if __name__ == "__main__":
    print(detect_storage().to_dict())

