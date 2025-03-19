import hashlib
import os
import types
import unittest

from yc_snapshot_client import CreateInfo, SnapshotClient, DeleteRequest


class TestClient(unittest.TestCase):
    def setUp(self):
        opts = [("grpc.max_receive_message_length", 100 * 1024 * 1024),
                ("grpc.max_send_message_length", 100 * 1024 * 1024)]
        self.client = SnapshotClient("localhost:17627", options=opts).__enter__()

    def tearDown(self):
        self.client.__exit__(None, None, None)

    def test_main(self):
        block = 4 * 1024
        blocks = 110
        create_info = CreateInfo(
            base="",
            base_project_id="",
            size=blocks * block,
            metadata="metadata",
            organization="",
            project_id="",
            disk="sda",
            name="name",
            description="desc",
            product_id="",
        )
        snapid = self.client.create(create_info)
        self.assertIsInstance(snapid, types.StringTypes)
        self.assertTrue(snapid)

        blobs = [(i * block, os.urandom(block)) for i in range(blocks)]
        for offset, blob in blobs:
            self.client.write(snapid, blob, offset)
        self.client.commit(snapid)

        reader = self.client.open(snapid)
        for offset, blob in blobs:
            self.assertEqual(reader.read(block, offset), blob)

        # Cross-block read
        self.assertEqual(reader.read(2*block, block/2), blobs[0][1][block/2:] + blobs[1][1] + blobs[2][1][:block/2])
        # Partial tail read
        self.assertEqual(reader.read(block, len(blobs) * block - block/2), blobs[len(blobs)-1][1][block/2:])

        with self.assertRaises(Exception):
            reader.read(block, (len(blobs) + 1) * block)

        listing = self.client.list_full()
        self.assertIn(snapid, [i.id for i in listing])

        listing = self.client.list_full(project_id="")
        self.assertIn(snapid, [i.id for i in listing])

        listing, _, _ = self.client.list(limit=1)
        self.assertEqual(len(listing), 1)

        listing = self.client.list_full(disk=create_info.disk)
        self.assertIn(snapid, [i.id for i in listing])

        listing = self.client.list_full(cursor=snapid)
        self.assertNotIn(snapid, [i.id for i in listing])

        info = self.client.info(snapid)
        self.assertEqual(info.id, snapid)
        self.assertEqual(info.name, create_info.name)
        self.assertEqual(info.base, create_info.base)
        self.assertEqual(info.size, create_info.size)
        self.assertEqual(info.disk, create_info.disk)
        self.assertEqual(info.metadata, create_info.metadata)

        imageh = hashlib.sha512()
        for offset, blob in blobs:
            h = hashlib.sha512()
            h.update(blob)
            hashsum = h.name + ":" + h.hexdigest()
            self.assertEqual(reader.get_block_checksum(block, offset),
                             hashsum)
            imageh.update(hashsum)

        self.assertEqual(reader.snapshot_checksum,
                         imageh.name + ":" + imageh.hexdigest())
        self.client.delete(DeleteRequest(snapid))
        listing = self.client.list_full()
        self.assertNotIn(snapid, [i.id for i in listing])
