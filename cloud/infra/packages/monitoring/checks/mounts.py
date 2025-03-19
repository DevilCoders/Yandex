#!/usr/bin/env python3
import re
import os.path
from itertools import chain
from typing import Dict, Generator, Tuple
from yc_monitoring import (
    JugglerPassiveCheck,
    JugglerPassiveCheckException,
)


CRIT_MOUNTS_GPU = {"/", "/var", "/boot"}
CRIT_MOUNTS_WO_GPU = CRIT_MOUNTS_GPU | {"/var/lib/yc/serverless"}
WARN_MOUNTS = {"/var/crashes"}
GPU_PLATFORM_FILE = "/etc/yc/infra/compute_gpu_platform.yaml"


def read_file(path: str, pattern) -> Generator[Dict, None, None]:
    """
        Returns dicts with parsed data from matched rows.
    """
    with open(path) as fd:
        for row in fd.readlines():
            match = pattern.match(row)
            if match:
                yield match.groupdict()


def get_crit_mounts():
    if not os.path.isfile(GPU_PLATFORM_FILE) or os.stat(GPU_PLATFORM_FILE).st_size == 0:
        return CRIT_MOUNTS_WO_GPU
    with open(GPU_PLATFORM_FILE) as fd:
        return CRIT_MOUNTS_WO_GPU if fd.read() == 'None' else CRIT_MOUNTS_GPU


def iter_fstab_mount_points() -> Generator[str, None, None]:
    pattern = re.compile(r'^[\w=/-]+[\t ]+(?P<mount_point>[\w/]+)[\t ]+.*')
    for item in read_file("/etc/fstab", pattern):
        try:
            yield item['mount_point']
        except KeyError:
            raise JugglerPassiveCheckException("No mount_point section in /etc/fstab. It might be broken")


def iter_mounted_mount_points() -> Generator[Tuple[str], None, None]:
    pattern = re.compile(r'^[\w/-]+[\t ]+(?P<mount_point>[\w/-]+)[\t ]\S+[\t ](?P<options>[\w/,=-]+)[\t ]+.*')
    for item in read_file("/proc/mounts", pattern):
        try:
            yield item['mount_point'], item['options'].split(",")
        except (KeyError, AttributeError):
            raise JugglerPassiveCheckException("No mount_point section in /proc/mounts")


def check_mounts(check: JugglerPassiveCheck):
    # get mount_points info
    fstab_mps = set(iter_fstab_mount_points())
    mounted_mps = dict(iter_mounted_mount_points())
    mounted_mps_opts = dict(iter_mounted_mount_points())
    crit_mounts = get_crit_mounts()

    # find absent mount points
    crit_mps_absent_in_fstab = crit_mounts.difference(fstab_mps)
    skip_items = set(mounted_mps).union(crit_mps_absent_in_fstab)
    crit_mps_not_mounted = list(
        filter(lambda mp: mp not in skip_items, crit_mounts))

    warn_mps_not_exist_in_fstab = WARN_MOUNTS.difference(fstab_mps)
    skip_items = set(mounted_mps).union(warn_mps_not_exist_in_fstab)
    warn_mps_not_mounted = list(filter(lambda mp: mp not in skip_items, WARN_MOUNTS))

    # find ro mount points
    crit_mps_mounted_ro = list(filter(
        lambda mp: mp in mounted_mps_opts and 'ro' in mounted_mps_opts[mp], set(
            chain(crit_mounts, WARN_MOUNTS)),
    ))

    # report if found are crit mount points
    if crit_mps_absent_in_fstab:
        check.crit(
            "Absent in fstab: {}".format(', '.join(chain(crit_mps_absent_in_fstab, warn_mps_not_exist_in_fstab)))
        )
    if crit_mps_not_mounted:
        check.crit("Not mounted: {}".format(', '.join(chain(crit_mps_not_mounted, warn_mps_not_mounted))))
    if crit_mps_mounted_ro:
        check.crit("Mounted FS is read-only: {}".format(', '.join(crit_mps_mounted_ro)))

    # report if found are warn mount points only
    if warn_mps_not_exist_in_fstab and not crit_mps_absent_in_fstab:
        check.warn("Absent in fstab: {}".format(', '.join(warn_mps_not_exist_in_fstab)))
    if warn_mps_not_mounted and not crit_mps_not_mounted:
        check.warn("Not mounted: {}".format(', '.join(warn_mps_not_mounted)))


def main():
    check = JugglerPassiveCheck("mounts")
    try:
        check_mounts(check)
    except JugglerPassiveCheckException as ex:
        check.crit(ex.description)
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == '__main__':
    main()
