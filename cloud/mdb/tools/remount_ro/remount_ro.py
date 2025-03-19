import sh
import logging

log = logging.getLogger(__name__)


def _remount(mount_point):
    return sh.mount('-o', 'remount,errors=remount-ro', mount_point)


def _disks() -> list:
    return ['/disks/' + disk_name for disk_name in sh.ls('/disks').split('\n') if disk_name]


def _remount_all():
    log.info('started remount')
    remount_points = ['/', '/data'] + _disks()
    for point in remount_points:
        if not point:
            continue
        _remount(point)


def _fstab_line_update(line: str) -> str:
    """
    to test:

    _fstab_line_update('/dev/md/2		/data	ext4	defaults,noatime	0 0')
    '/dev/md/2\t\t/data\text4\tdefaults,noatime,errors=remount-ro\t0\t0'
    _fstab_line_update('proc /proc')
    'proc /proc'
    _fstab_line_update('UUID=6fdc1d7c-8ad2-4dd0-ba70-579405687456	/		ext4	defaults	0	1')
    'UUID=6fdc1d7c-8ad2-4dd0-ba70-579405687456	/		ext4	defaults	0	1'
    _fstab_line_update('UUID=19238d1d-0483-4618-8a34-e97a1063dae5 /disks/19238d1d-0483-4618-8a34-e97a1063dae5 ext4 defaults,noatime 0 0')
    'UUID=19238d1d-0483-4618-8a34-e97a1063dae5 /disks/19238d1d-0483-4618-8a34-e97a1063dae5 ext4 defaults,noatime 0 0'
    _fstab_line_update('# / was on /dev/md1 during installation')
    '# / was on /dev/md1 during installation'
    """
    EXT4 = 'ext4'
    if EXT4 not in line:
        return line
    if 'errors=remount-ro' in line:
        log.info('already present %s', line)
        return line
    before_ext4, after_ext4 = line.split(EXT4)
    after_ext4 = after_ext4.strip()
    mount_opts, backup, fscheck = after_ext4.split()
    mount_opts, fscheck = mount_opts.strip(), fscheck.strip()
    if mount_opts:
        mount_opts += ','
    mount_opts += 'errors=remount-ro'
    result = f'{before_ext4}{EXT4}\t{mount_opts}\t{backup}\t{fscheck}'
    return result.strip()


def _update_fstab():
    result = []
    with open('/etc/fstab', 'r') as fp:
        for line in fp.readlines():
            result.append(_fstab_line_update(line))
    sh.mv('/etc/fstab', '/etc/fstab.MDB-8802.back')
    open('/etc/fstab', 'w').writelines(result)


def main():
    """
    Remount with errors=remount-ro and update fs tab
    """
    _remount_all()
    _update_fstab()
