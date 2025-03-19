#!/usr/bin/env python
# encoding: utf-8

NEW_COUNTERS_KEYS = ['simple_objects_count', 'simple_objects_size', 'multipart_objects_count', 'multipart_objects_size',
                     'objects_parts_count', 'objects_parts_size', 'deleted_objects_count', 'deleted_objects_size',
                     'active_multipart_count']


class Chunk(object):
    def __init__(self, bid, cid,
                 split_key=None,
                 split_limit=None,
                 start_key=None,
                 end_key=None,
                 created=None,
                 shard_id=None,
                 updated_ts=None,
                 bucket_name=None,
                 old_counters=None):
        self.bid = bid
        self.cid = cid
        self.bucket_name = bucket_name
        # key from which objects will be moved to another chunk
        self.split_key = split_key
        # max objects count which will be moved to new chunk
        self.split_limit = split_limit
        # start and end keys
        self.start_key = start_key
        self.end_key = end_key
        # creation timestamp
        self.created = created
        self.shard_id = shard_id
        # updated timestamp
        self.updated_ts = updated_ts
        # old counters - counters from s3.chunks

    def __str__(self):
        return '({bid}, {cid})'.format(bid=self.bid, cid=self.cid)
