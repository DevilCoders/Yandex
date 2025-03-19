Feature: Testing service buckets stats
  Background: Set initial data
    Given empty DB

    Given buckets owner account "1"
      And a bucket with name "Columba"
      And we add object "Phact" with following attributes
          """
          data_size: 1111
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns-1
          mds_couple_id: 111
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111
          """

      And a bucket with name "Leo"
      And we add object "Adhafera" with following attributes
          """
          data_size: 2222
          data_md5: 22222222-2222-2222-2222-222222222222
          mds_namespace: ns-2
          mds_couple_id: 222
          mds_key_version: 2
          mds_key_uuid: 22222222-2222-2222-2222-222222222222
          """
      And we add object "Denebola" with following attributes
          """
          data_size: 3333
          data_md5: 33333333-3333-3333-3333-333333333333
          mds_namespace: ns-3
          mds_couple_id: 333
          mds_key_version: 3
          mds_key_uuid: 33333333-3333-3333-3333-333333333333
          """

    Given buckets owner account "3"
      And a bucket with name "Hydra"
      And we add object "Alphard" with following attributes
          """
          data_size: 4444
          data_md5: 44444444-4444-4444-4444-444444444444
          mds_namespace: ns-4
          mds_couple_id: 444
          mds_key_version: 4
          mds_key_uuid: 44444444-4444-4444-4444-444444444444
          """
      And we add object "Hydrobius" with following attributes
          """
          data_size: 5555
          data_md5: 55555555-5555-5555-5555-555555555555
          mds_namespace: ns-5
          mds_couple_id: 555
          mds_key_version: 5
          mds_key_uuid: 55555555-5555-5555-5555-555555555555
          """
      And we add object "Minchir" with following attributes
          """
          data_size: 6666
          data_md5: 66666666-6666-6666-6666-666666666666
          mds_namespace: ns-6
          mds_couple_id: 666
          mds_key_version: 6
          mds_key_uuid: 66666666-6666-6666-6666-666666666666
          """

    Given buckets owner account "2"
      And a bucket with name "Delphinus"
      And we add object "Musica" with following attributes
          """
          data_size: 7777
          data_md5: 77777777-7777-7777-7777-777777777777
          mds_namespace: ns-7
          mds_couple_id: 777
          mds_key_version: 7
          mds_key_uuid: 77777777-7777-7777-7777-777777777777
          """

     When we refresh all statistic

  Scenario: Request buckets stats for existing owner
    Then buckets stats for owner "1":
          """
          - name: Leo
            multipart_objects_count: 0
            multipart_objects_size: 0
            objects_parts_count: 0
            objects_parts_size: 0
            service_id: 1
            simple_objects_count: 2
            simple_objects_size: 5555

          - name: Columba
            multipart_objects_count: 0
            multipart_objects_size: 0
            objects_parts_count: 0
            objects_parts_size: 0
            service_id: 1
            simple_objects_count: 1
            simple_objects_size: 1111
          """

  Scenario: Request buckets stats for nonexisting owner
    Then buckets stats for owner "42":
          """
          """

  Scenario: Request all buckets stats
    Then buckets stats for all buckets:
          """
          - name: Hydra
            multipart_objects_count: 0
            multipart_objects_size: 0
            objects_parts_count: 0
            objects_parts_size: 0
            service_id: 3
            simple_objects_count: 3
            simple_objects_size: 16665

          - name: Delphinus
            multipart_objects_count: 0
            multipart_objects_size: 0
            objects_parts_count: 0
            objects_parts_size: 0
            service_id: 2
            simple_objects_count: 1
            simple_objects_size: 7777

          - name: Leo
            multipart_objects_count: 0
            multipart_objects_size: 0
            objects_parts_count: 0
            objects_parts_size: 0
            service_id: 1
            simple_objects_count: 2
            simple_objects_size: 5555

          - name: Columba
            multipart_objects_count: 0
            multipart_objects_size: 0
            objects_parts_count: 0
            objects_parts_size: 0
            service_id: 1
            simple_objects_count: 1
            simple_objects_size: 1111
          """
