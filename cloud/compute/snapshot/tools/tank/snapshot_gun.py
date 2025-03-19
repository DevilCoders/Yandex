import logging
import random
import os
import sys
import json
import time
import google.protobuf as pb
import traceback

# print sys.path

from snapshot import SnapshotClient, CreateInfo, CreateMetadata

opts = [("grpc.max_receive_message_length", 100 * 1024 * 1024),
        ("grpc.max_send_message_length", 100 * 1024 * 1024)]
projects = ["tank-{}".format(i) for i in xrange(16)]

log = logging.getLogger(__name__)


def craft_create_info(base="", size=0):
    return CreateInfo(base=base,
                      base_project_id="",
                      size=size,
                      metadata="",
                      organization="Tank",
                      project_id=random.choice(projects),
                      disk="",
                      name="tank-{}".format(size),
                      product_id="random")

def craft_create_metadata(filename):
     return CreateMetadata(metadata="",
                           organization="Tank",
                           project_id=random.choice(projects),
                           product_id="random")

def craft_list_request(d):
    if "created_before" in d:
        t = time.time()
        d[u"created_before"] = pb.timestamp_pb2.Timestamp(seconds=int(t), nanos=int((t-int(t))*10**9))
    return {str(k): v for k, v in d.iteritems()}


class LoadTest(object):
    def __init__(self, gun):

        # you'll be able to call gun's methods using this field:
        self.gun = gun
 
    def create_base(self, missile):
        # we use gun's measuring context to measure time.
        # The results will be aggregated automatically:
        # log.info("create_base: %s", missile)
        d = json.loads(missile)
        size = d["size"] * self.chunk_size
        create_info = craft_create_info(base="", size=size)
        
        with self.gun.measure("create_base/create"):
            create_info = craft_create_info(base="", size=size)
            s_id = self.grpc_client.create(create_info)
            log.debug("create_base: %s", s_id)

        for i in xrange(0, size, self.chunk_size):
            with self.gun.measure("create_base/write"):
                self.grpc_client.write(s_id, create_info.project_id, self.data, i)

        with self.gun.measure("create_base/commit"):
            self.grpc_client.commit(s_id, create_info.project_id)

        with self.gun.measure("create_base/info"):
            self.grpc_client.info(s_id, create_info.project_id)

    def convert(self, missile):
        # log.info("convert: %s", missile)
        d = json.loads(missile)
        create_metadata = craft_create_metadata(d["key"])
        
        with self.gun.measure("convert"):
            s_id = self.grpc_client.convert(create_metadata=create_metadata, fmt="raw", **d)

        log.debug("convert: success %s", s_id)

    def list(self, missile):
        # log.info("list: %s", missile)
        d = craft_list_request(json.loads(missile))

        cnt = "_".join(["list_"] + sorted(d))

        with self.gun.measure(cnt):
            log.debug(d)
            l = self.grpc_client.list(**d)

    def list_full(self, missile):
        #log.info("list_full: %s", missile)
        d = craft_list_request(json.loads(missile))

        cnt = "_".join(["list_full_"] + sorted(d))

        with self.gun.measure(cnt):
            l = self.grpc_client.list_full(**d)

        # query new snapshots as well
        if "project_id" in missile and missile["project_id"] == self.info_project:
            self.snapshot_list = l

    def info(self, missile):
        sh = random.choice(self.snapshot_list)

        with self.gun.measure("info"):
            self.grpc_client.info(sh.id, sh.project_id)

    def setup(self, param):
        ''' this will be executed in each worker before the test starts '''
        log.info("Setting up LoadTest: %s", param)
        target = param

        self.grpc_client = SnapshotClient(target, options=opts)
        self.chunk_size = int(self.gun.get_option("chunk_size", 4*1024*1024))

        #data_src = self.gun.get_option("data_src", "/dev/sda1")
        #with open(data_src) as f:
        #    self.data = f.read(self.chunk_size/2)

        with open("/dev/urandom") as f:
            self.data = f.read(self.chunk_size)

        self.info_project = random.choice(projects)
        self.snapshot_list = self.grpc_client.list_full(project_id=self.info_project)

    def teardown(self):
        ''' this will be executed in each worker after the end of the test '''
        log.info("Tearing down LoadTest")
        os._exit(0)
