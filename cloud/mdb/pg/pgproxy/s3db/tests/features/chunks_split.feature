Feature: Chunks splitting

  Background: Set buckets owner account
    Given buckets owner account "1"

  Scenario: Splitting chunk
    Given empty DB
    Given a bucket with name "ChunkSplitting"
    When we add object "test/Object1" with following attributes:
        """
        data_size: 1111
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    When we add object "test/Object1" with following attributes:
        """
        data_size: 2222
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Given a multipart upload in bucket "ChunkSplitting" for object "test/Object2"
    When we upload part for object "test/Object2":
        """
        part_id: 1
        data_size: 3333
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 33333333-3333-3333-3333-333333333333
        """
    When we upload part for object "test/Object2":
        """
        part_id: 1
        data_size: 4444
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 44444444-4444-4444-4444-444444444444
        """
    Given a multipart upload in bucket "ChunkSplitting" for object "test/Object3"
    When we upload part for object "test/Object3":
        """
        part_id: 1
        data_size: 5555
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 55555555-5555-5555-5555-555555555555
        """
    When we complete the following multipart upload:
        """
        name: test/Object3
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        parts_data:
            - part_id: 1
              data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        """
    Then bucket "ChunkSplitting" has "1" object(s) of size "2222"
     And bucket "ChunkSplitting" has "1" multipart object(s) of size "5555"
     And bucket "ChunkSplitting" has "1" object part(s) of size "4444"
     And bucket "ChunkSplitting" in delete queue has "1" object(s) of size "1111"
     And bucket "ChunkSplitting" in delete queue has "1" object part(s) of size "3333"
    When we add object "test/Object4" with following attributes:
        """
        data_size: 6666
        data_md5: 66666666-6666-6666-6666-666666666666
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 66666666-6666-6666-6666-666666666666
        """
    When we add object "test/Object4" with following attributes:
        """
        data_size: 7777
        data_md5: 77777777-7777-7777-7777-777777777777
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 77777777-7777-7777-7777-777777777777
        """
    Given a multipart upload in bucket "ChunkSplitting" for object "test/Object8"
    When we upload part for object "test/Object8":
        """
        part_id: 1
        data_size: 8888
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 88888888-8888-8888-8888-888888888888
        """
    When we upload part for object "test/Object8":
        """
        part_id: 1
        data_size: 9999
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 99999999-9999-9999-9999-999999999999
        """
    Given a multipart upload in bucket "ChunkSplitting" for object "test/Object6"
    When we upload part for object "test/Object6":
        """
        part_id: 1
        data_size: 1111
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    When we complete the following multipart upload:
        """
        name: test/Object6
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        parts_data:
            - part_id: 1
              data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        """
    When we add object "test/Object7" with following attributes:
        """
        data_size: 2222
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then bucket "ChunkSplitting" has "3" object(s) of size "12221"
     And bucket "ChunkSplitting" has "2" multipart object(s) of size "6666"
     And bucket "ChunkSplitting" has "2" object part(s) of size "14443"
     And bucket "ChunkSplitting" in delete queue has "2" object(s) of size "7777"
     And bucket "ChunkSplitting" in delete queue has "2" object part(s) of size "12221"
    When we update chunk counters on "0" db
     And we update chunk counters on "1" db
    When we split chunks bigger than "4" object(s) on "0" db
     And we split chunks bigger than "4" object(s) on "1" db
    Then bucket "ChunkSplitting" has "3" object(s) of size "12221"
     And bucket "ChunkSplitting" has "2" multipart object(s) of size "6666"
     And bucket "ChunkSplitting" has "2" object part(s) of size "14443"
     And bucket "ChunkSplitting" in delete queue has "2" object(s) of size "7777"
     And bucket "ChunkSplitting" in delete queue has "2" object part(s) of size "12221"
     And bucket "ChunkSplitting" consists of "2" chunks and has "3" object(s) of size "12221"
    When we list all chunks counters in the bucket "ChunkSplitting"
    Then we get following objects:
        """
        - simple_objects_count: 2
          simple_objects_size: 9999
          multipart_objects_count: 1
          multipart_objects_size: 5555
          objects_parts_count: 1
          objects_parts_size: 4444

        - simple_objects_count: 1
          simple_objects_size: 2222
          multipart_objects_count: 1
          multipart_objects_size: 1111
          objects_parts_count: 1
          objects_parts_size: 9999
        """
    When we list all objects in a bucket with prefix "NULL", delimiter "NULL" and start_after "NULL"
    Then we get following objects:
        """
        - name: test/Object1
          data_size: 2222
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns-2
          mds_couple_id: 222
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222

        - name: test/Object3
          data_size: 5555
          data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
          mds_namespace:
          parts_count: 1

        - name: test/Object4
          data_size: 7777
          data_md5: 77777777-7777-7777-7777-777777777777
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 77777777-7777-7777-7777-777777777777

        - name: test/Object6
          data_size: 1111
          data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
          mds_namespace:
          parts_count: 1

        - name: test/Object7
          data_size: 2222
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    When we list current parts for object "test/Object2"
    Then we get the following object part:
        """
        name: test/Object2
        part_id: 1
        data_size: 4444
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 44444444-4444-4444-4444-444444444444
        """
    When we list current parts for object "test/Object8"
    Then we get the following object part:
        """
        name: test/Object8
        part_id: 1
        data_size: 9999
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 99999999-9999-9999-9999-999999999999
        """
    When we update chunk counters on "0" db
     And we update chunk counters on "1" db
    When we split chunks bigger than "2" object(s) on "0" db
     And we split chunks bigger than "2" object(s) on "1" db
    Then bucket "ChunkSplitting" consists of "3" chunks and has "3" object(s) of size "12221"
    When we list all chunks counters in the bucket "ChunkSplitting"
    Then we get following objects:
        """
        - simple_objects_count: 1
          simple_objects_size: 2222
          multipart_objects_count: 1
          multipart_objects_size: 5555
          objects_parts_count: 1
          objects_parts_size: 4444

        - simple_objects_count: 1
          simple_objects_size: 7777
          multipart_objects_count: 0
          multipart_objects_size: 0
          objects_parts_count: 0
          objects_parts_size: 0

        - simple_objects_count: 1
          simple_objects_size: 2222
          multipart_objects_count: 1
          multipart_objects_size: 1111
          objects_parts_count: 1
          objects_parts_size: 9999
        """
    When we list all objects in a bucket with prefix "NULL", delimiter "NULL" and start_after "NULL"
    Then we get following objects:
        """
        - name: test/Object1
          data_size: 2222
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns-2
          mds_couple_id: 222
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222

        - name: test/Object3
          data_size: 5555
          data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
          mds_namespace:
          parts_count: 1

        - name: test/Object4
          data_size: 7777
          data_md5: 77777777-7777-7777-7777-777777777777
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 77777777-7777-7777-7777-777777777777

        - name: test/Object6
          data_size: 1111
          data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
          mds_namespace:
          parts_count: 1

        - name: test/Object7
          data_size: 2222
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    When we update chunk counters on "0" db
     And we update chunk counters on "1" db
    When we split chunks bigger than "1" object(s) on "0" db
     And we split chunks bigger than "1" object(s) on "1" db
    Then bucket "ChunkSplitting" consists of "5" chunks and has "3" object(s) of size "12221"
    When we list all chunks counters in the bucket "ChunkSplitting"
    Then we get following objects:
        """
        - simple_objects_count: 1
          simple_objects_size: 2222
          multipart_objects_count: 0
          multipart_objects_size: 0
          objects_parts_count: 1
          objects_parts_size: 4444

        - simple_objects_count: 0
          simple_objects_size: 0
          multipart_objects_count: 1
          multipart_objects_size: 5555
          objects_parts_count: 0
          objects_parts_size: 0

        - simple_objects_count: 1
          simple_objects_size: 7777
          multipart_objects_count: 0
          multipart_objects_size: 0
          objects_parts_count: 0
          objects_parts_size: 0

        - simple_objects_count: 0
          simple_objects_size: 0
          multipart_objects_count: 1
          multipart_objects_size: 1111
          objects_parts_count: 0
          objects_parts_size: 0

        - simple_objects_count: 1
          simple_objects_size: 2222
          multipart_objects_count: 0
          multipart_objects_size: 0
          objects_parts_count: 1
          objects_parts_size: 9999
        """
    When we update chunks counters on "0" meta db
     And we update chunks counters on "1" meta db
    Then chunks counters for bucket "ChunkSplitting" on meta and db are the same
    When we list all chunks in the bucket "ChunkSplitting" on meta db
    Then we get following chunks
        """
        - start_key: NULL
          end_key:  test/Object3
          shard_id: 0

        - start_key: test/Object3
          end_key: test/Object4
          shard_id: 0

        - start_key: test/Object4
          end_key: test/Object6
          shard_id: 0

        - start_key: test/Object6
          end_key: test/Object7
          shard_id: 0

        - start_key: test/Object7
          end_key: NULL
          shard_id: 0
        """
    When we refresh all statistic
    When we run chunk_mover on "0" meta db
     And we run chunk_mover on "1" meta db
    When we list all chunks in the bucket "ChunkSplitting" on meta db
    Then we get following chunks
        """
        - start_key: NULL
          end_key:  test/Object3
          shard_id: 0

        - start_key: test/Object3
          end_key: test/Object4
          shard_id: 0

        - start_key: test/Object4
          end_key: test/Object6
          shard_id: 0

        - start_key: test/Object6
          end_key: test/Object7
          shard_id: 0

        - start_key: test/Object7
          end_key: NULL
          shard_id: 1
        """
    When we list all objects in a bucket with prefix "test/", delimiter "NULL" and start_after "NULL"
    Then we get following objects:
        """
        - name: test/Object1
          data_size: 2222
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns-2
          mds_couple_id: 222
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222

        - name: test/Object3
          data_size: 5555
          data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
          mds_namespace:
          parts_count: 1

        - name: test/Object4
          data_size: 7777
          data_md5: 77777777-7777-7777-7777-777777777777
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 77777777-7777-7777-7777-777777777777

        - name: test/Object6
          data_size: 1111
          data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
          mds_namespace:
          parts_count: 1

        - name: test/Object7
          data_size: 2222
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    When we add object "test/Object66" with following attributes:
        """
        data_size: 6666
        data_md5: 66666666-6666-6666-6666-666666666666
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 66666666-6666-6666-6666-666666666666
        """
    When we update chunks counters on "0" meta db
     And we update chunks counters on "1" meta db
    When we refresh shards statistics on "0" meta db
     And we refresh shards statistics on "1" meta db
    When we run chunk_mover on "0" meta db
     And we run chunk_mover on "1" meta db
    When we list all chunks in the bucket "ChunkSplitting" on meta db
    Then we get following chunks
        """
        - start_key: NULL
          end_key:  test/Object3
          shard_id: 0

        - start_key: test/Object3
          end_key: test/Object4
          shard_id: 0

        - start_key: test/Object4
          end_key: test/Object6
          shard_id: 0

        - start_key: test/Object6
          end_key: test/Object7
          shard_id: 1

        - start_key: test/Object7
          end_key: NULL
          shard_id: 1
        """
    Then bucket "ChunkSplitting" consists of "5" chunks and has "4" object(s) of size "18887"
    When we add object "test/Object00" with following attributes:
        """
        data_size: 1111
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    When we add object "test/Object000" with following attributes:
        """
        data_size: 1111
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    When we update chunk counters on "0" db
     And we update chunk counters on "1" db
    When we list all chunks counters in the bucket "ChunkSplitting"
    Then we get following objects:
        """
        - simple_objects_count: 3
          simple_objects_size: 4444
          multipart_objects_count: 0
          multipart_objects_size: 0
          objects_parts_count: 1
          objects_parts_size: 4444

        - simple_objects_count: 0
          simple_objects_size: 0
          multipart_objects_count: 1
          multipart_objects_size: 5555
          objects_parts_count: 0
          objects_parts_size: 0

        - simple_objects_count: 1
          simple_objects_size: 7777
          multipart_objects_count: 0
          multipart_objects_size: 0
          objects_parts_count: 0
          objects_parts_size: 0

        - simple_objects_count: 1
          simple_objects_size: 6666
          multipart_objects_count: 1
          multipart_objects_size: 1111
          objects_parts_count: 0
          objects_parts_size: 0

        - simple_objects_count: 1
          simple_objects_size: 2222
          multipart_objects_count: 0
          multipart_objects_size: 0
          objects_parts_count: 1
          objects_parts_size: 9999
        """
    When we update chunks counters on "0" meta db
     And we update chunks counters on "1" meta db
    When we refresh shards statistics on "0" meta db
     And we refresh shards statistics on "1" meta db
    When we run chunk_mover on "0" meta db
     And we run chunk_mover on "1" meta db
    When we list all chunks in the bucket "ChunkSplitting" on meta db
    Then we get following chunks
        """
        - start_key: NULL
          end_key:  test/Object3
          shard_id: 0

        - start_key: test/Object3
          end_key: test/Object4
          shard_id: 0

        - start_key: test/Object4
          end_key: test/Object6
          shard_id: 1

        - start_key: test/Object6
          end_key: test/Object7
          shard_id: 1

        - start_key: test/Object7
          end_key: NULL
          shard_id: 1
        """
    When we update chunks counters on "0" meta db
     And we update chunks counters on "1" meta db
    When we refresh shards statistics on "0" meta db
     And we refresh shards statistics on "1" meta db
    When we run chunk_mover on "0" meta db
     And we run chunk_mover on "1" meta db
    When we list all chunks in the bucket "ChunkSplitting" on meta db
    Then we get following chunks
        """
        - start_key: NULL
          end_key:  test/Object3
          shard_id: 0

        - start_key: test/Object3
          end_key: test/Object4
          shard_id: 0

        - start_key: test/Object4
          end_key: test/Object6
          shard_id: 1

        - start_key: test/Object6
          end_key: test/Object7
          shard_id: 1

        - start_key: test/Object7
          end_key: NULL
          shard_id: 1
        """
    Then bucket "ChunkSplitting" consists of "5" chunks and has "6" object(s) of size "21109"
    When we list all chunks counters in the bucket "ChunkSplitting"
    Then we get following objects:
        """
        - simple_objects_count: 3
          simple_objects_size: 4444
          multipart_objects_count: 0
          multipart_objects_size: 0
          objects_parts_count: 1
          objects_parts_size: 4444

        - simple_objects_count: 0
          simple_objects_size: 0
          multipart_objects_count: 1
          multipart_objects_size: 5555
          objects_parts_count: 0
          objects_parts_size: 0

        - simple_objects_count: 1
          simple_objects_size: 7777
          multipart_objects_count: 0
          multipart_objects_size: 0
          objects_parts_count: 0
          objects_parts_size: 0

        - simple_objects_count: 1
          simple_objects_size: 6666
          multipart_objects_count: 1
          multipart_objects_size: 1111
          objects_parts_count: 0
          objects_parts_size: 0

        - simple_objects_count: 1
          simple_objects_size: 2222
          multipart_objects_count: 0
          multipart_objects_size: 0
          objects_parts_count: 1
          objects_parts_size: 9999
        """
    When we list all objects in a bucket with prefix "NULL", delimiter "/" and start_after "NULL"
    Then we get following objects:
        """
        - name: test/
          data_size:
          data_md5:
          mds_namespace:
          mds_couple_id:
          mds_key_version:
          mds_key_uuid:
        """
    When we list all objects in a bucket with prefix "t", delimiter "NULL" and start_after "NULL"
    Then we get following objects:
        """
        - name: test/Object00
          data_size: 1111
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - name: test/Object000
          data_size: 1111
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - name: test/Object1
          data_size: 2222
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns-2
          mds_couple_id: 222
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222

        - name: test/Object3
          data_size: 5555
          data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
          mds_namespace:
          parts_count: 1

        - name: test/Object4
          data_size: 7777
          data_md5: 77777777-7777-7777-7777-777777777777
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 77777777-7777-7777-7777-777777777777

        - name: test/Object6
          data_size: 1111
          data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
          mds_namespace:
          parts_count: 1

        - name: test/Object66
          data_size: 6666
          data_md5: 66666666-6666-6666-6666-666666666666
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 66666666-6666-6666-6666-666666666666

        - name: test/Object7
          data_size: 2222
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    When we list all objects in a bucket with prefix "t", delimiter "NULL" and start_after "test/Object00"
    Then we get following objects:
        """
        - name: test/Object000
          data_size: 1111
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - name: test/Object1
          data_size: 2222
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns-2
          mds_couple_id: 222
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222

        - name: test/Object3
          data_size: 5555
          data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
          mds_namespace:
          parts_count: 1

        - name: test/Object4
          data_size: 7777
          data_md5: 77777777-7777-7777-7777-777777777777
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 77777777-7777-7777-7777-777777777777

        - name: test/Object6
          data_size: 1111
          data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
          mds_namespace:
          parts_count: 1

        - name: test/Object66
          data_size: 6666
          data_md5: 66666666-6666-6666-6666-666666666666
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 66666666-6666-6666-6666-666666666666

        - name: test/Object7
          data_size: 2222
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
     And we have no errors in counters

  Scenario: Splitting chunk by key
    Given a bucket with name "ChunkSplittingKey"
    When we add object "test/Object1" with following attributes:
        """
        data_size: 1111
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    When we add object "test/Object1" with following attributes:
        """
        data_size: 2222
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Given a multipart upload in bucket "ChunkSplittingKey" for object "test/Object2"
    When we upload part for object "test/Object2":
        """
        part_id: 1
        data_size: 3333
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 33333333-3333-3333-3333-333333333333
        """
    When we upload part for object "test/Object2":
        """
        part_id: 1
        data_size: 4444
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 44444444-4444-4444-4444-444444444444
        """
    Given a multipart upload in bucket "ChunkSplittingKey" for object "test/Object3"
    When we upload part for object "test/Object3":
        """
        part_id: 1
        data_size: 5555
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 55555555-5555-5555-5555-555555555555
        """
    When we complete the following multipart upload:
        """
        name: test/Object3
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        parts_data:
            - part_id: 1
              data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        """
    Then bucket "ChunkSplittingKey" has "1" object(s) of size "2222"
     And bucket "ChunkSplittingKey" has "1" multipart object(s) of size "5555"
     And bucket "ChunkSplittingKey" has "1" object part(s) of size "4444"
     And bucket "ChunkSplittingKey" in delete queue has "1" object(s) of size "1111"
     And bucket "ChunkSplittingKey" in delete queue has "1" object part(s) of size "3333"
    When we add object "test/Object4" with following attributes:
        """
        data_size: 6666
        data_md5: 66666666-6666-6666-6666-666666666666
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 66666666-6666-6666-6666-666666666666
        """
    When we add object "test/Object4" with following attributes:
        """
        data_size: 7777
        data_md5: 77777777-7777-7777-7777-777777777777
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 77777777-7777-7777-7777-777777777777
        """
    Given a multipart upload in bucket "ChunkSplittingKey" for object "test/Object5"
    When we upload part for object "test/Object5":
        """
        part_id: 1
        data_size: 8888
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 88888888-8888-8888-8888-888888888888
        """
    When we upload part for object "test/Object5":
        """
        part_id: 1
        data_size: 9999
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 99999999-9999-9999-9999-999999999999
        """
    Given a multipart upload in bucket "ChunkSplittingKey" for object "test/Object6"
    When we upload part for object "test/Object6":
        """
        part_id: 1
        data_size: 1111
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    When we complete the following multipart upload:
        """
        name: test/Object6
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        parts_data:
            - part_id: 1
              data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        """
    When we add object "test/Object7" with following attributes:
        """
        data_size: 2222
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    Then bucket "ChunkSplittingKey" has "3" object(s) of size "12221"
     And bucket "ChunkSplittingKey" has "2" multipart object(s) of size "6666"
     And bucket "ChunkSplittingKey" has "2" object part(s) of size "14443"
     And bucket "ChunkSplittingKey" in delete queue has "2" object(s) of size "7777"
     And bucket "ChunkSplittingKey" in delete queue has "2" object part(s) of size "12221"
    When we update chunk counters on "0" db
     And we update chunk counters on "1" db
    When we split chunk by key "test/Object4"
    Then bucket "ChunkSplittingKey" has "3" object(s) of size "12221"
     And bucket "ChunkSplittingKey" has "2" multipart object(s) of size "6666"
     And bucket "ChunkSplittingKey" has "2" object part(s) of size "14443"
     And bucket "ChunkSplittingKey" in delete queue has "2" object(s) of size "7777"
     And bucket "ChunkSplittingKey" in delete queue has "2" object part(s) of size "12221"
     And bucket "ChunkSplittingKey" consists of "2" chunks and has "3" object(s) of size "12221"
    When we list all chunks counters in the bucket "ChunkSplittingKey"
    Then we get following objects:
        """
        - simple_objects_count: 1
          simple_objects_size: 2222
          multipart_objects_count: 1
          multipart_objects_size: 5555
          objects_parts_count: 1
          objects_parts_size: 4444

        - simple_objects_count: 2
          simple_objects_size: 9999
          multipart_objects_count: 1
          multipart_objects_size: 1111
          objects_parts_count: 1
          objects_parts_size: 9999
        """
    When we list all objects in a bucket with prefix "NULL", delimiter "NULL" and start_after "NULL"
    Then we get following objects:
        """
        - name: test/Object1
          data_size: 2222
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns-2
          mds_couple_id: 222
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222

        - name: test/Object3
          data_size: 5555
          data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
          mds_namespace:
          parts_count: 1

        - name: test/Object4
          data_size: 7777
          data_md5: 77777777-7777-7777-7777-777777777777
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 77777777-7777-7777-7777-777777777777

        - name: test/Object6
          data_size: 1111
          data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
          mds_namespace:
          parts_count: 1

        - name: test/Object7
          data_size: 2222
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns
          mds_couple_id: 123
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    When we list current parts for object "test/Object2"
    Then we get the following object part:
        """
        name: test/Object2
        part_id: 1
        data_size: 4444
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 44444444-4444-4444-4444-444444444444
        """
    When we list current parts for object "test/Object5"
    Then we get the following object part:
        """
        name: test/Object5
        part_id: 1
        data_size: 9999
        data_md5: b056467a-2b1c-42b7-8cb3-d7af2476ae2e
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 99999999-9999-9999-9999-999999999999
        """
    When we update chunks counters on "0" meta db
     And we update chunks counters on "1" meta db
    Then chunks counters for bucket "ChunkSplittingKey" on meta and db are the same
     And we have no errors in counters

  Scenario: Splitting chunk with emulating database failure on 1 step
    Given a bucket with name "ChunkSplittingFail"
    When we add object "test/Object1" with following attributes:
        """
        data_size: 1111
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    When we add object "test/Object2" with following attributes:
        """
        data_size: 2222
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    When we add object "test/Object3" with following attributes:
        """
        data_size: 3333
        data_md5: 33333333-3333-3333-3333-333333333333
        mds_namespace: ns-3
        mds_couple_id: 333
        mds_key_version: 1
        mds_key_uuid: 33333333-3333-3333-3333-333333333333
        """
    When we update chunk counters on "0" db
     And we update chunk counters on "1" db
    When we fail before all commits while split chunks bigger than "1" object(s) on "0" db
     And we fail before all commits while split chunks bigger than "1" object(s) on "1" db
    Then we have no prepared transactions
    When we list all objects in a bucket with prefix "NULL", delimiter "NULL" and start_after "NULL"
    Then we get following objects:
        """
        - data_size: 1111
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns-1
          mds_couple_id: 111
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - data_size: 2222
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns-2
          mds_couple_id: 222
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222

        - data_size: 3333
          data_md5: 33333333-3333-3333-3333-333333333333
          mds_namespace: ns-3
          mds_couple_id: 333
          mds_key_version: 1
          mds_key_uuid: 33333333-3333-3333-3333-333333333333
        """
    When we update chunk counters on "0" db
     And we update chunk counters on "1" db
    When we fail after "1" transaction commits while split chunks bigger than "2" object(s) on "0" db
     And we fail after "1" transaction commits while split chunks bigger than "2" object(s) on "1" db
    Then bucket "ChunkSplittingFail" consists of "2" chunks and has "3" object(s) of size "6666"
     And we have no prepared transactions
    When we list all objects in a bucket with prefix "NULL", delimiter "NULL" and start_after "NULL"
    Then we get following objects:
        """
        - data_size: 1111
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns-1
          mds_couple_id: 111
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - data_size: 2222
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns-2
          mds_couple_id: 222
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222

        - data_size: 3333
          data_md5: 33333333-3333-3333-3333-333333333333
          mds_namespace: ns-3
          mds_couple_id: 333
          mds_key_version: 1
          mds_key_uuid: 33333333-3333-3333-3333-333333333333
        """
     And we have no errors in counters

  Scenario: Splitting chunk with emulating database failure on 2 step
    Given a bucket with name "ChunkSplittingFail2"
    When we add object "test/Object1" with following attributes:
        """
        data_size: 1111
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
    When we add object "test/Object2" with following attributes:
        """
        data_size: 2222
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
    When we add object "test/Object3" with following attributes:
        """
        data_size: 3333
        data_md5: 33333333-3333-3333-3333-333333333333
        mds_namespace: ns-3
        mds_couple_id: 333
        mds_key_version: 1
        mds_key_uuid: 33333333-3333-3333-3333-333333333333
        """
    When we update chunk counters on "0" db
     And we update chunk counters on "1" db
    When we fail after "2" transaction commits while split chunks bigger than "1" object(s) on "0" db
     And we fail after "2" transaction commits while split chunks bigger than "1" object(s) on "1" db
    Then we have no prepared transactions
    When we list all objects in a bucket with prefix "NULL", delimiter "NULL" and start_after "NULL"
    Then we get following objects:
        """
        - data_size: 1111
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns-1
          mds_couple_id: 111
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - data_size: 2222
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns-2
          mds_couple_id: 222
          mds_key_version: 1
          mds_key_uuid: 22222222-2222-2222-2222-222222222222

        - data_size: 3333
          data_md5: 33333333-3333-3333-3333-333333333333
          mds_namespace: ns-3
          mds_couple_id: 333
          mds_key_version: 1
          mds_key_uuid: 33333333-3333-3333-3333-333333333333
        """
     And we have no errors in counters
