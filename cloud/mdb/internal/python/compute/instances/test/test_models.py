from yandex.cloud.priv.compute.v1 import instance_pb2
from cloud.mdb.internal.python.compute.instances import models


class TestAttachedDisk:
    def test_from_raw(self):
        """MDB-11423"""
        assert models.AttachedDisk.from_api(
            instance_pb2.AttachedDisk(
                auto_delete=True,
                device_name='disk-name',
                mode=instance_pb2.AttachedDisk.Mode.READ_WRITE,
                disk_id='disk-id',
                status=instance_pb2.AttachedDisk.Status.DETACH_ERROR,
            )
        ) == models.AttachedDisk(
            auto_delete=True,
            device_name='disk-name',
            mode=models.DiskMode.READ_WRITE,
            disk_id='disk-id',
            status=models.DiskStatus.DETACH_ERROR,
        )
