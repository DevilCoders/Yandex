Feature: Testing sharding schema

  Scenario: Refreshing shard statistic works as expected
    Given empty DB
     When we list shards statistics
     Then we get following shards statistics:
         """
         - shard_id: 0
           buckets_count: 0
           chunks_count: 0

         - shard_id: 1
           buckets_count: 0
           chunks_count: 0
         """
     When we add a bucket with name "foo" of account "1"
     When we list shards statistics
     Then we get following shards statistics:
         """
         - shard_id: 0
           buckets_count: 0
           chunks_count: 0

         - shard_id: 1
           buckets_count: 0
           chunks_count: 0
         """
     When we refresh shards statistics on "0" meta db
      And we refresh shards statistics on "1" meta db
     When we list shards statistics
     Then we get following shards statistics:
         """
         - shard_id: 0
           buckets_count: 1
           chunks_count: 1

         - shard_id: 1
           buckets_count: 0
           chunks_count: 0
         """
     When we add a bucket with name "bar" of account "1"
      And we refresh shards statistics on "0" meta db
      And we refresh shards statistics on "1" meta db
     When we list shards statistics
     Then we get following shards statistics:
         """
         - shard_id: 0
           buckets_count: 1
           chunks_count: 1

         - shard_id: 1
           buckets_count: 1
           chunks_count: 1
         """
     When we add object "foo" with following attributes:
         """
         data_size: 1111
         data_md5: 11111111-1111-1111-1111-111111111111
         mds_namespace: ns-1
         mds_couple_id: 111
         mds_key_version: 1
         mds_key_uuid: 11111111-1111-1111-1111-111111111111
         """
     When we add object "bar" with following attributes:
         """
         data_size: 2222
         data_md5: 22222222-2222-2222-2222-222222222222
         mds_namespace: ns-2
         mds_couple_id: 222
         mds_key_version: 1
         mds_key_uuid: 22222222-2222-2222-2222-222222222222
         """
     When we update chunk counters on "0" db
      And we update chunk counters on "1" db
     When we split chunks bigger than "1" object(s) on "1" db
      And we refresh shards statistics on "0" meta db
      And we refresh shards statistics on "1" meta db
     When we list shards statistics
     Then we get following shards statistics:
         """
         - shard_id: 0
           buckets_count: 1
           chunks_count: 1

         - shard_id: 1
           buckets_count: 1
           chunks_count: 2
         """
