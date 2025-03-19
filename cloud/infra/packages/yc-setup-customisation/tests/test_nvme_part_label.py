#!/usr/bin/env python3
from unittest import mock, TestCase
from bin import nvme_part_label
import json
from parameterized import parameterized


class TestNvmePartLabel(TestCase):
    @staticmethod
    def mock_lsblk_output():
        return json.dumps({
            "blockdevices": [
                {"name": "sdf", "parttype": "null", "partlabel": "null", "fstype": "null", "rota": "1",
                 "children": [
                     {"name": "sdf1", "parttype": "0fc63daf-8483-4772-8e79-3d69d8477de4", "partlabel": "ROTKIKIMR04",
                      "fstype": "null", "rota": "1"}
                 ]
                 },
                {"name": "nvme0n1", "parttype": "null", "partlabel": "null", "fstype": "null", "rota": "0",
                 "children": [
                     {"name": "nvme0n1p1", "parttype": "0fc63daf-8483-4772-8e79-3d69d8477de4",
                      "partlabel": "NVMEKIKIMR01", "fstype": "null", "rota": "0"}
                 ]
                 },
                {"name": "sdd", "parttype": "null", "partlabel": "null", "fstype": "null", "rota": "1",
                 "children": [
                     {"name": "sdd2", "parttype": "0fc63daf-8483-4772-8e79-3d69d8477de4", "partlabel": "primary",
                      "fstype": "linux_raid_member", "rota": "1",
                      "children": [
                          {"name": "md3", "parttype": "null", "partlabel": "null", "fstype": "ext4", "rota": "1"}
                      ]
                      },
                     {"name": "sdd3", "parttype": "0fc63daf-8483-4772-8e79-3d69d8477de4", "partlabel": "primary",
                      "fstype": "linux_raid_member", "rota": "1",
                      "children": [
                          {"name": "md6", "parttype": "null", "partlabel": "null", "fstype": "ext4", "rota": "1"}
                      ]
                      },
                     {"name": "sdd1", "parttype": "21686148-6449-6e6f-744e-656564454649", "partlabel": "primary",
                      "fstype": "null", "rota": "1"}
                 ]
                 }
            ]
        })

    @staticmethod
    def mock_lsblk_output_invalid_blockdevices():
        return json.dumps({"MOCKdevices": [
            {"name": "sdf", "parttype": "null", "partlabel": "null", "fstype": "null", "rota": "1",
             "children": [
                 {"name": "sdf1", "parttype": "0fc63daf-8483-4772-8e79-3d69d8477de4", "partlabel": "ROTKIKIMR04",
                  "fstype": "null", "rota": "1"}
             ]}]})

    @staticmethod
    def mock_lsblk_output_missing_name():
        return json.dumps({"blockdevices": [
            {"parttype": "null", "partlabel": "null", "fstype": "null", "rota": "1",
             "children": [
                 {"name": "sdf1", "parttype": "0fc63daf-8483-4772-8e79-3d69d8477de4", "partlabel": "ROTKIKIMR04",
                  "fstype": "null", "rota": "1"}
             ]}]})

    @staticmethod
    def mock_lsblk_output_wrong_children():
        return json.dumps({"blockdevices": [
            {"name": "sda", "parttype": "null", "partlabel": "null", "fstype": "null", "rota": "1"},
            {"name": "sdb", "parttype": "null", "partlabel": "null", "fstype": "null", "rota": "1",
             "children": "wrong_type"},
            {"name": "sdc", "parttype": "null", "partlabel": "null", "fstype": "null", "rota": "1",
             "children": [
                 {"name": "sdc1", "parttype": "0fc63daf-8483-4772-8e79-3d69d8477de4", "fstype": "null", "rota": "1"}
             ]},
        ]})

    @parameterized.expand([
        ("sdf", {"sdf": "ROTKIKIMR04"}),
        ("nvme0n1", {"nvme0n1": "NVMEKIKIMR01"}),
        ("sdd", {"sdd": "primary"}),
        ("fake", {})
    ])
    def test_get_partitions_labels(self, disk, expected_result):
        with mock.patch.object(nvme_part_label, "exec_commands") as mock_exec_commands:
            mock_exec_commands.return_value = self.mock_lsblk_output()
            self.assertEqual(nvme_part_label.get_partitions_labels(disk), expected_result)

    def test_get_partitions_labels_wrong_blockdevices(self):
        with mock.patch.object(nvme_part_label, "exec_commands") as mock_exec_commands:
            mock_exec_commands.return_value = self.mock_lsblk_output_invalid_blockdevices()
            with self.assertRaises(nvme_part_label.FieldMissingError):
                nvme_part_label.get_partitions_labels("sda")

    def test_get_partitions_labels_missing_name(self):
        with mock.patch.object(nvme_part_label, "exec_commands") as mock_exec_commands:
            mock_exec_commands.return_value = self.mock_lsblk_output_missing_name()
            with self.assertRaises(nvme_part_label.FieldMissingError):
                nvme_part_label.get_partitions_labels("name")

    @parameterized.expand([
        ("missing children", "sda"),
        ("wrong children field type", "sdb"),
        ("missing partlabel field", "sdc")
    ])
    def test_get_partitions_labels_wrong_children(self, check_name, disk):
        with mock.patch.object(nvme_part_label, "exec_commands") as mock_exec_commands:
            mock_exec_commands.return_value = self.mock_lsblk_output_wrong_children()
            with self.assertRaises(nvme_part_label.FieldMissingError):
                nvme_part_label.get_partitions_labels(disk)
