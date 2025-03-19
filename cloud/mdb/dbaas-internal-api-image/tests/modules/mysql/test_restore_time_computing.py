"""
Restore time computing tests
"""
import pytest
from datetime import datetime, timedelta
from dbaas_internal_api.utils.types import (
    Backup,
    ClusterBackup,
    DTYPE_LOCAL_SSD,
    DTYPE_NETWORK_HDD,
    DTYPE_NETWORK_SSD,
    DTYPE_NRD,
    ExistedHostResources,
    GIGABYTE,
    MEGABYTE,
    TERABYTE,
    VTYPE_COMPUTE,
    VTYPE_PORTO,
)
from dbaas_internal_api.utils.time import compute_restore_time_limit_2

# pylint: disable=missing-docstring, invalid-name


def getDefaultFlavor():
    return {
        'io_limit': 400 * MEGABYTE,
        'vtype': VTYPE_PORTO,
        'network_limit': 400 * MEGABYTE,
        'cpu_guarantee': 2,
        'cpu_fraction': 100,
    }


def getDefaultHostResources() -> ExistedHostResources:
    return ExistedHostResources(disk_size=48 * GIGABYTE, disk_type_id=DTYPE_LOCAL_SSD)


class Tests_restore_time_computing:
    def test_time_earlier_Backup_start_fails(self):
        with pytest.raises(RuntimeError):
            flavor = getDefaultFlavor()
            backup = Backup("some_id", datetime(2021, 1, 1, 12, 00), datetime(2021, 1, 1, 12, 30))

            compute_restore_time_limit_2(
                dest_flavor=flavor,
                backup=backup,
                time=datetime(2021, 1, 1, 11),
                src_resources=getDefaultHostResources(),
                dest_resources=getDefaultHostResources(),
            )

    def test_backup_uses_disk_size(self):
        flavor = getDefaultFlavor()
        backup = Backup("some_id", datetime(2021, 1, 1, 12, 00), datetime(2021, 1, 1, 12, 30))

        result = compute_restore_time_limit_2(
            dest_flavor=flavor,
            backup=backup,
            time=datetime(2021, 1, 1, 13),
            src_resources=getDefaultHostResources(),
            dest_resources=getDefaultHostResources(),
        )
        assert result == timedelta(seconds=11352, microseconds=960000)

    def test_use_disk_size_when_uncompressed_unknown(self):
        flavor = getDefaultFlavor()
        backup = ClusterBackup("some_id", datetime(2021, 1, 1, 12, 00), datetime(2021, 1, 1, 12, 30))

        result = compute_restore_time_limit_2(
            dest_flavor=flavor,
            backup=backup,
            time=datetime(2021, 1, 1, 13),
            src_resources=getDefaultHostResources(),
            dest_resources=getDefaultHostResources(),
        )
        assert result == timedelta(seconds=11352, microseconds=960000)

    def test_use_uncompressed_size(self):
        flavor = getDefaultFlavor()
        backup = ClusterBackup(
            "some_id", datetime(2021, 1, 1, 12, 00), datetime(2021, 1, 1, 12, 30), uncompressed_size=100 * GIGABYTE
        )

        result = compute_restore_time_limit_2(
            dest_flavor=flavor,
            backup=backup,
            time=datetime(2021, 1, 1, 13),
            src_resources=getDefaultHostResources(),
            dest_resources=getDefaultHostResources(),
        )
        assert result == timedelta(seconds=11952)

    def test_use_network_limit(self):
        flavor = getDefaultFlavor()
        flavor['network_limit'] = 10 * MEGABYTE
        backup = ClusterBackup(
            "some_id",
            datetime(2021, 1, 1, 12, 00),
            datetime(2021, 1, 1, 12, 30),
            size=30 * GIGABYTE,
            uncompressed_size=100 * GIGABYTE,
        )

        result = compute_restore_time_limit_2(
            dest_flavor=flavor,
            backup=backup,
            time=datetime(2021, 1, 1, 13),
            src_resources=getDefaultHostResources(),
            dest_resources=getDefaultHostResources(),
        )
        assert result == timedelta(seconds=24624)

    def test_use_software_io_limit(self):
        flavor = getDefaultFlavor()
        flavor['vtype'] = VTYPE_COMPUTE
        backup = ClusterBackup(
            "some_id",
            datetime(2021, 1, 1, 12, 00),
            datetime(2021, 1, 1, 12, 30),
            size=30 * GIGABYTE,
            uncompressed_size=100 * GIGABYTE,
        )
        dest_resources = ExistedHostResources(disk_size=2 * TERABYTE, disk_type_id=DTYPE_NETWORK_SSD)

        result = compute_restore_time_limit_2(
            dest_flavor=flavor,
            backup=backup,
            time=datetime(2021, 1, 1, 13),
            src_resources=getDefaultHostResources(),
            dest_resources=dest_resources,
            software_io_limit=65 * MEGABYTE,
        )
        assert result == timedelta(seconds=17889, microseconds=230766)

    @pytest.mark.parametrize(
        ['disk_size', 'disk_type', 'expected_timedelta'],
        [
            (100 * GIGABYTE, DTYPE_NETWORK_HDD, timedelta(seconds=26160)),
            (100 * GIGABYTE, DTYPE_NETWORK_SSD, timedelta(seconds=18480)),
            # 500 < 400 test network limit, use network limit
            (100 * GIGABYTE, DTYPE_LOCAL_SSD, timedelta(seconds=11721, microseconds=600000)),
            (100 * GIGABYTE, DTYPE_NRD, timedelta(seconds=13609, microseconds=756098)),
            # after disk size 450MB/15MB * 32GB we should use disk limit of 450MB/s for network ssd
            (2 * TERABYTE, DTYPE_NETWORK_SSD, timedelta(seconds=11824, microseconds=2)),
            (3 * TERABYTE, DTYPE_NETWORK_SSD, timedelta(seconds=11824, microseconds=2)),
        ],
    )
    def test_use_formula(self, disk_size, disk_type, expected_timedelta):
        flavor = getDefaultFlavor()
        flavor['vtype'] = VTYPE_COMPUTE
        backup = ClusterBackup(
            "some_id",
            datetime(2021, 1, 1, 12, 00),
            datetime(2021, 1, 1, 12, 30),
            size=30 * GIGABYTE,
            uncompressed_size=100 * GIGABYTE,
        )
        dest_resources = ExistedHostResources(disk_size=disk_size, disk_type_id=disk_type)

        result = compute_restore_time_limit_2(
            dest_flavor=flavor,
            backup=backup,
            time=datetime(2021, 1, 1, 13),
            src_resources=getDefaultHostResources(),
            dest_resources=dest_resources,
        )
        assert result == expected_timedelta
