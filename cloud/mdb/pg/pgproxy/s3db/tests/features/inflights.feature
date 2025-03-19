Feature: Inflights operation

    Background: Setup bucket
    Given empty DB
        And buckets owner account "2"
        And a bucket with name "inflights"

    Scenario: Start inflight
        When we add object "started" with following attributes:
        """
        data_size: 50
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        storage_class: 0
        """
        When we start inflight for object "started":
        """
        mds_namespace:
        mds_couple_id:
        mds_key_version:
        mds_key_uuid:
        """
        When we add inflight for object "started" with part_id "1":
        """
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-2222-1111-1111-111111111111
        """
        Then inflight has part "1" for object "started":
        """
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-2222-1111-1111-111111111111
        """
        And inflight has parts count of "1" for object "started"
        And we have no errors in counters


    Scenario: Add inflight without starting it
        When we add object "second" with following attributes:
        """
        data_size: 50
        data_md5: 11111111-2222-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-2222-3333-1111-111111111111
        storage_class: 0
        """
        When we add inflight for object "second" with part_id "0":
        """
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-2222-3333-1111-111111111111
        """
        Then inflight operation completed with error or returned empty result
         And inflight has parts count of "0" for object "second"
         And we have no errors in counters
        When we list all in the storage delete queue
        Then we get the following deleted object(s) as a result:
          """
          - name: "second"
            part_id: 0
            mds_couple_id: 123
            mds_key_version: 1
            mds_key_uuid: 11111111-2222-3333-1111-111111111111
          """


    Scenario: Start inflight and add few parts
        When we start inflight for object "third":
        """
        mds_namespace:
        mds_couple_id:
        mds_key_version:
        mds_key_uuid:
        """
        When we add inflight for object "third" with part_id "1":
        """
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-3333-1111-1111-111111111111
        metadata: '{"key":"value"}'
        """
        Then inflight has part "1" for object "third":
        """
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-3333-1111-1111-111111111111
        metadata: '{"key":"value"}'
        """
        And inflight has parts count of "1" for object "third"
        When we add inflight for object "third" with part_id "2":
        """
        mds_namespace: ns
        mds_couple_id: 1234
        mds_key_version: 2
        mds_key_uuid: 11111111-4444-1111-1111-111111111111
        """
        Then inflight has part "2" for object "third":
        """
        mds_namespace: ns
        mds_couple_id: 1234
        mds_key_version: 2
        mds_key_uuid: 11111111-4444-1111-1111-111111111111
        """
        And inflight has parts count of "2" for object "third"
        And we have no errors in counters


    Scenario: Start inflight with few parts and abort it
        When we start inflight for object "fourth":
        """
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 0
        mds_key_uuid: 11111111-0000-1111-1111-111111111111
        """
        Then inflight has part "0" for object "fourth":
        """
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 0
        mds_key_uuid: 11111111-0000-1111-1111-111111111111
        """
        And inflight has parts count of "0" for object "fourth"
        When we add inflight for object "fourth" with part_id "1":
        """
        mds_namespace: ns
        mds_couple_id: 12345
        mds_key_version: 2
        mds_key_uuid: 11111111-2222-1111-1111-111111111111
        """
        Then inflight has part "1" for object "fourth":
        """
        mds_namespace: ns
        mds_couple_id: 12345
        mds_key_version: 2
        mds_key_uuid: 11111111-2222-1111-1111-111111111111
        """
        And inflight has parts count of "1" for object "fourth"
        When we abort inflight for object "fourth"
        Then there are no inflights for object "fourth"
        And we have no errors in counters
        When we list all in the storage delete queue
        Then we get the following deleted object(s) as a result:
          """
          - name: "fourth"
            part_id: 0
            mds_couple_id: 123
            mds_key_version: 0
            mds_key_uuid: 11111111-0000-1111-1111-111111111111

          - name: "fourth"
            part_id: 1
            mds_couple_id: 12345
            mds_key_version: 2
            mds_key_uuid: 11111111-2222-1111-1111-111111111111
          """


    Scenario: Complete not started inflight
      When we failed to complete not started inflight upload for object "Bilbo"
      Then we have no errors in counters


    Scenario: Complete started inflight
        Given a multipart upload in bucket "inflights" for object "Bilbo"
        When we upload part for object "Bilbo":
        """
        part_id: 1
        data_size: 7000000
        data_md5: b056467a-1111-42b7-8cb3-d7af2476ae21
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
        When we upload part for object "Bilbo":
        """
        part_id: 2
        data_size: 7000000
        data_md5: b056467a-2222-42b7-8cb3-d7af2476ae22
        mds_namespace: ns
        mds_couple_id: 1234
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
        When we upload part for object "Bilbo":
        """
        part_id: 3
        data_size: 7
        data_md5: b056467a-3333-42b7-8cb3-d7af2476ae23
        mds_namespace: ns
        mds_couple_id: 1235
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
        When we complete the following multipart upload:
        """
        name: Bilbo
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_data:
            - part_id: 1
              data_md5: b056467a-1111-42b7-8cb3-d7af2476ae21

            - part_id: 2
              data_md5: b056467a-2222-42b7-8cb3-d7af2476ae22

            - part_id: 3
              data_md5: b056467a-3333-42b7-8cb3-d7af2476ae23
        """
        Then our object has attributes:
        """
        name: Bilbo
        data_size: 14000007
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_count: 3
        """
        And we have no errors in counters
        When we start inflight for object "Bilbo":
        """
        mds_couple_id: 1
        mds_key_version: 2
        mds_key_uuid: 11111111-4444-1111-1111-111111111111
        """
        Then inflight has part "0" for object "Bilbo":
        """
        mds_couple_id: 1
        mds_key_version: 2
        mds_key_uuid: 11111111-4444-1111-1111-111111111111
        """
        Then inflight has parts count of "0" for object "Bilbo"
        When we add inflight for object "Bilbo" with part_id "1":
        """
        mds_namespace: ns
        mds_couple_id: 1234
        mds_key_version: 2
        mds_key_uuid: 11111111-5555-1111-1111-111111111111
        """
        Then inflight has parts count of "1" for object "Bilbo"
         And inflight has part "1" for object "Bilbo":
        """
        mds_couple_id: 1234
        mds_key_version: 2
        mds_key_uuid: 11111111-5555-1111-1111-111111111111
        """
        When we add inflight for object "Bilbo" with part_id "2":
        """
        mds_namespace: ns
        mds_couple_id: 12345
        mds_key_version: 3
        mds_key_uuid: 11111111-6666-1111-1111-111111111111
        """
        Then inflight has part "2" for object "Bilbo":
        """
        mds_namespace: ns
        mds_couple_id: 12345
        mds_key_version: 3
        mds_key_uuid: 11111111-6666-1111-1111-111111111111
        """
        Then inflight has parts count of "2" for object "Bilbo"
        When we add inflight for object "Bilbo" with part_id "3":
        """
        mds_namespace: ns
        mds_couple_id: 123456
        mds_key_version: 3
        mds_key_uuid: 11111111-7777-1111-1111-111111111111
        """
        Then inflight has part "3" for object "Bilbo":
        """
        mds_namespace: ns
        mds_couple_id: 123456
        mds_key_version: 3
        mds_key_uuid: 11111111-7777-1111-1111-111111111111
        """
        Then inflight has parts count of "3" for object "Bilbo"
        When we complete inflight upload for object "Bilbo" w/o errors
        Then inflight has parts count of "0" for object "Bilbo"
         And we have no errors in counters


    Scenario: Multipart inflight completion
        Given a multipart upload in bucket "inflights" for object "Frodo"
        When we upload part for object "Frodo":
        """
        part_id: 1
        data_size: 7000000
        data_md5: b056467a-1111-42b7-8cb3-d7af2476ae21
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
        When we upload part for object "Frodo":
        """
        part_id: 2
        data_size: 7000000
        data_md5: b056467a-2222-42b7-8cb3-d7af2476ae22
        mds_namespace: ns
        mds_couple_id: 1234
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-2222-1111-111111111111
        """
        When we upload part for object "Frodo":
        """
        part_id: 3
        data_size: 7
        data_md5: b056467a-3333-42b7-8cb3-d7af2476ae23
        mds_namespace: ns
        mds_couple_id: 12345
        mds_key_version: 3
        mds_key_uuid: 11111111-1111-3333-1111-111111111111
        """
        When we complete the following multipart upload:
        """
        name: Frodo
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_data:
            - part_id: 1
              data_md5: b056467a-1111-42b7-8cb3-d7af2476ae21

            - part_id: 2
              data_md5: b056467a-2222-42b7-8cb3-d7af2476ae22

            - part_id: 3
              data_md5: b056467a-3333-42b7-8cb3-d7af2476ae23
        """
        Then we have no errors in counters
        When we start inflight for object "Frodo":
        """
        mds_couple_id:
        mds_key_version:
        mds_key_uuid:
        metadata: '{"encryption": {"key_id": "ring"}}'
        """
        Then inflight has parts count of "0" for object "Frodo"
        When we add inflight for object "Frodo" with part_id "1":
        """
        mds_namespace: ns
        mds_couple_id: 1234
        mds_key_version: 2
        mds_key_uuid: 11111111-5555-1111-1111-111111111111
        metadata: '{"encryption": {"meta": "Frodo-1"}}'
        """
        Then inflight has parts count of "1" for object "Frodo"
        When we add inflight for object "Frodo" with part_id "2":
        """
        mds_namespace: ns
        mds_couple_id: 12345
        mds_key_version: 3
        mds_key_uuid: 11111111-6666-1111-1111-111111111111
        metadata: '{"encryption": {"meta": "Frodo-2"}}'
        """
        Then inflight has parts count of "2" for object "Frodo"
        When we add inflight for object "Frodo" with part_id "3":
        """
        mds_namespace: ns
        mds_couple_id: 123456
        mds_key_version: 4
        mds_key_uuid: 11111111-7777-1111-1111-111111111111
        metadata: '{"encryption": {"meta": "Frodo-3"}}'
        """
        When we complete inflight upload for object "Frodo" w/o errors
        Then object "Frodo" has fields:
         """
         mds_couple_id:
         mds_key_version:
         mds_key_uuid:

         metadata: '{"encryption": {"key_id": "ring", "parts_meta": ["Frodo-1", "Frodo-2", "Frodo-3"]}}'
         parts_count: 3
         parts_data:
           - part_id: 1
             mds_couple_id: 1234
             mds_key_version: 2
             mds_key_uuid: 11111111-5555-1111-1111-111111111111

           - part_id: 2
             mds_couple_id: 12345
             mds_key_version: 3
             mds_key_uuid: 11111111-6666-1111-1111-111111111111

           - part_id: 3
             mds_couple_id: 123456
             mds_key_version: 4
             mds_key_uuid: 11111111-7777-1111-1111-111111111111
         """
         And inflight has parts count of "0" for object "Frodo"
         And we have no errors in counters
        When we list all in the storage delete queue
        Then we get the following deleted object(s) as a result:
          """

          - name: "Frodo"
            part_id: 1
            data_md5: b056467a-1111-42b7-8cb3-d7af2476ae21
            mds_couple_id: 123
            mds_key_version: 1
            mds_key_uuid: 11111111-1111-1111-1111-111111111111

          - name: "Frodo"
            part_id: 2
            data_md5: b056467a-2222-42b7-8cb3-d7af2476ae22
            mds_couple_id: 1234
            mds_key_version: 2
            mds_key_uuid: 11111111-1111-2222-1111-111111111111

          - name: "Frodo"
            part_id: 3
            data_md5: b056467a-3333-42b7-8cb3-d7af2476ae23
            mds_couple_id: 12345
            mds_key_version: 3
            mds_key_uuid: 11111111-1111-3333-1111-111111111111

          """


    Scenario: Multipart inflight completion with empty metadata
        Given a multipart upload in bucket "inflights" for object "Sauron"
        When we upload part for object "Sauron":
        """
        part_id: 1
        data_size: 7000000
        data_md5: b056467a-1111-42b7-8cb3-d7af2476ae21
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
        When we upload part for object "Sauron":
        """
        part_id: 2
        data_size: 7000000
        data_md5: b056467a-2222-42b7-8cb3-d7af2476ae22
        mds_namespace: ns
        mds_couple_id: 1234
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-2222-1111-111111111111
        """
        When we upload part for object "Sauron":
        """
        part_id: 3
        data_size: 7
        data_md5: b056467a-3333-42b7-8cb3-d7af2476ae23
        mds_namespace: ns
        mds_couple_id: 12345
        mds_key_version: 3
        mds_key_uuid: 11111111-1111-3333-1111-111111111111
        """
        When we complete the following multipart upload:
        """
        name: Sauron
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_data:
            - part_id: 1
              data_md5: b056467a-1111-42b7-8cb3-d7af2476ae21

            - part_id: 2
              data_md5: b056467a-2222-42b7-8cb3-d7af2476ae22

            - part_id: 3
              data_md5: b056467a-3333-42b7-8cb3-d7af2476ae23
        """
        Then we have no errors in counters
        When we start inflight for object "Sauron":
        """
        mds_couple_id:
        mds_key_version:
        mds_key_uuid:
        """
        Then inflight has parts count of "0" for object "Sauron"
        When we add inflight for object "Sauron" with part_id "1":
        """
        mds_namespace: ns
        mds_couple_id: 1234
        mds_key_version: 2
        mds_key_uuid: 11111111-5555-1111-1111-111111111111
        metadata: null
        """
        Then inflight has parts count of "1" for object "Sauron"
        When we add inflight for object "Sauron" with part_id "2":
        """
        mds_namespace: ns
        mds_couple_id: 12345
        mds_key_version: 3
        mds_key_uuid: 11111111-6666-1111-1111-111111111111
        metadata: null
        """
        Then inflight has parts count of "2" for object "Sauron"
        When we add inflight for object "Sauron" with part_id "3":
        """
        mds_namespace: ns
        mds_couple_id: 123456
        mds_key_version: 4
        mds_key_uuid: 11111111-7777-1111-1111-111111111111
        metadata: null
        """
        When we complete inflight upload for object "Sauron" w/o errors
        Then object "Sauron" has fields:
         """
         mds_couple_id:
         mds_key_version:
         mds_key_uuid:

         metadata: null
         parts_count: 3
         parts_data:
           - part_id: 1
             mds_couple_id: 1234
             mds_key_version: 2
             mds_key_uuid: 11111111-5555-1111-1111-111111111111

           - part_id: 2
             mds_couple_id: 12345
             mds_key_version: 3
             mds_key_uuid: 11111111-6666-1111-1111-111111111111

           - part_id: 3
             mds_couple_id: 123456
             mds_key_version: 4
             mds_key_uuid: 11111111-7777-1111-1111-111111111111
         """
         And inflight has parts count of "0" for object "Sauron"
         And we have no errors in counters
        When we list all in the storage delete queue
        Then we get the following deleted object(s) as a result:
          """

          - name: "Sauron"
            part_id: 1
            data_md5: b056467a-1111-42b7-8cb3-d7af2476ae21
            mds_couple_id: 123
            mds_key_version: 1
            mds_key_uuid: 11111111-1111-1111-1111-111111111111

          - name: "Sauron"
            part_id: 2
            data_md5: b056467a-2222-42b7-8cb3-d7af2476ae22
            mds_couple_id: 1234
            mds_key_version: 2
            mds_key_uuid: 11111111-1111-2222-1111-111111111111

          - name: "Sauron"
            part_id: 3
            data_md5: b056467a-3333-42b7-8cb3-d7af2476ae23
            mds_couple_id: 12345
            mds_key_version: 3
            mds_key_uuid: 11111111-1111-3333-1111-111111111111

          """


    Scenario: Multipart inflight completion with partly empty metadata
        Given a multipart upload in bucket "inflights" for object "Sauron"
        When we upload part for object "Sauron":
        """
        part_id: 1
        data_size: 7000000
        data_md5: b056467a-1111-42b7-8cb3-d7af2476ae21
        mds_namespace: ns
        mds_couple_id: 123
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
        When we upload part for object "Sauron":
        """
        part_id: 2
        data_size: 7000000
        data_md5: b056467a-2222-42b7-8cb3-d7af2476ae22
        mds_namespace: ns
        mds_couple_id: 1234
        mds_key_version: 2
        mds_key_uuid: 11111111-1111-2222-1111-111111111111
        """
        When we upload part for object "Sauron":
        """
        part_id: 3
        data_size: 7
        data_md5: b056467a-3333-42b7-8cb3-d7af2476ae23
        mds_namespace: ns
        mds_couple_id: 12345
        mds_key_version: 3
        mds_key_uuid: 11111111-1111-3333-1111-111111111111

        """
        When we complete the following multipart upload:
        """
        name: Sauron
        data_md5: 66666666-6666-6666-6666-666666666666
        parts_data:
            - part_id: 1
              data_md5: b056467a-1111-42b7-8cb3-d7af2476ae21

            - part_id: 2
              data_md5: b056467a-2222-42b7-8cb3-d7af2476ae22

            - part_id: 3
              data_md5: b056467a-3333-42b7-8cb3-d7af2476ae23
        """
        Then we have no errors in counters
        When we start inflight for object "Sauron":
        """
        mds_couple_id:
        mds_key_version:
        mds_key_uuid:
        metadata: '{"encryption": {"key_id": "ring"}}'
        """
        Then inflight has parts count of "0" for object "Sauron"
        When we add inflight for object "Sauron" with part_id "1":
        """
        mds_namespace: ns
        mds_couple_id: 1234
        mds_key_version: 2
        mds_key_uuid: 11111111-5555-1111-1111-111111111111
        metadata: null
        """
        Then inflight has parts count of "1" for object "Sauron"
        When we add inflight for object "Sauron" with part_id "2":
        """
        mds_namespace: ns
        mds_couple_id: 12345
        mds_key_version: 3
        mds_key_uuid: 11111111-6666-1111-1111-111111111111
        metadata: '{"encryption": {"meta": "Sauron-2"}}'
        """
        Then inflight has parts count of "2" for object "Sauron"
        When we add inflight for object "Sauron" with part_id "3":
        """
        mds_namespace: ns
        mds_couple_id: 123456
        mds_key_version: 4
        mds_key_uuid: 11111111-7777-1111-1111-111111111111
        metadata: null
        """
        When we complete inflight upload for object "Sauron" w/o errors
        Then object "Sauron" has fields:
         """
         mds_couple_id:
         mds_key_version:
         mds_key_uuid:

         metadata: '{"encryption": {"key_id": "ring", "parts_meta": [null, "Sauron-2", null]}}'
         parts_count: 3
         parts_data:
           - part_id: 1
             mds_couple_id: 1234
             mds_key_version: 2
             mds_key_uuid: 11111111-5555-1111-1111-111111111111

           - part_id: 2
             mds_couple_id: 12345
             mds_key_version: 3
             mds_key_uuid: 11111111-6666-1111-1111-111111111111

           - part_id: 3
             mds_couple_id: 123456
             mds_key_version: 4
             mds_key_uuid: 11111111-7777-1111-1111-111111111111
         """
         And inflight has parts count of "0" for object "Sauron"
         And we have no errors in counters
        When we list all in the storage delete queue
        Then we get the following deleted object(s) as a result:
          """

          - name: "Sauron"
            part_id: 1
            data_md5: b056467a-1111-42b7-8cb3-d7af2476ae21
            mds_couple_id: 123
            mds_key_version: 1
            mds_key_uuid: 11111111-1111-1111-1111-111111111111

          - name: "Sauron"
            part_id: 2
            data_md5: b056467a-2222-42b7-8cb3-d7af2476ae22
            mds_couple_id: 1234
            mds_key_version: 2
            mds_key_uuid: 11111111-1111-2222-1111-111111111111

          - name: "Sauron"
            part_id: 3
            data_md5: b056467a-3333-42b7-8cb3-d7af2476ae23
            mds_couple_id: 12345
            mds_key_version: 3
            mds_key_uuid: 11111111-1111-3333-1111-111111111111

          """


    Scenario: Multipart inflight completion with non-empty object's metadata
      Given a multipart upload in bucket "inflights" for object "Gimli"
      When we add multipart object "Gimli" with following attributes
        """
        data_md5: 99999999-9999-9999-9999-999999999999

        storage_class: 0
        metadata: '{"encryption": {"extra_key": 100500}}'

        parts:
            - part_id: 1
              mds_namespace: ns-1
              mds_couple_id: 1
              mds_key_version: 1
              mds_key_uuid: 11111111-1111-1111-1111-111111111111
              data_size: 1000000000
              data_md5: 11111111-1111-1111-1111-111111111111

            - part_id: 2
              mds_namespace: ns-2
              mds_couple_id: 2
              mds_key_version: 2
              mds_key_uuid: 22222222-2222-2222-2222-222222222222
              data_size: 2000000000
              data_md5: 22222222-2222-2222-2222-222222222222
        """
        Then we have no errors in counters
        When we start inflight for object "Gimli":
        """
        mds_couple_id:
        mds_key_version:
        mds_key_uuid:
        metadata: '{"encryption": {"key_id": "ring"}}'
        """
        Then inflight has parts count of "0" for object "Gimli"
        When we add inflight for object "Gimli" with part_id "1":
        """
        mds_namespace: ns
        mds_couple_id: 1234
        mds_key_version: 2
        mds_key_uuid: 11111111-5555-1111-1111-111111111111
        metadata: null
        """
        Then inflight has parts count of "1" for object "Gimli"
        When we add inflight for object "Gimli" with part_id "2":
        """
        mds_namespace: ns
        mds_couple_id: 12345
        mds_key_version: 3
        mds_key_uuid: 11111111-6666-1111-1111-111111111111
        metadata: '{"encryption": {"meta": "Gimli-2"}}'
        """
        When we complete inflight upload for object "Gimli" w/o errors
        Then object "Gimli" has fields:
         """
         mds_couple_id:
         mds_key_version:
         mds_key_uuid:

         metadata: '{"encryption": {"key_id": "ring", "parts_meta": [null, "Gimli-2"]}}'

         parts_count: 2
         parts_data:
           - part_id: 1
             mds_couple_id: 1234
             mds_key_version: 2
             mds_key_uuid: 11111111-5555-1111-1111-111111111111

           - part_id: 2
             mds_couple_id: 12345
             mds_key_version: 3
             mds_key_uuid: 11111111-6666-1111-1111-111111111111

         """
         And inflight has parts count of "0" for object "Gimli"
         And we have no errors in counters
        When we list all in the storage delete queue
        Then we get the following deleted object(s) as a result:
          """

          - name: "Gimli"
            part_id: 1
            data_md5: 11111111-1111-1111-1111-111111111111
            mds_couple_id: 1
            mds_key_version: 1
            mds_key_uuid: 11111111-1111-1111-1111-111111111111

          - name: "Gimli"
            part_id: 2
            data_md5: 22222222-2222-2222-2222-222222222222
            mds_couple_id: 2
            mds_key_version: 2
            mds_key_uuid: 22222222-2222-2222-2222-222222222222

          """


    Scenario: Simple object inflight completion
        When we add object "Gandalf" with following attributes:
        """
        data_size: 50
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-0000-1111-1111-111111111111
        storage_class: 0
        """
        When we start inflight for object "Gandalf":
        """
        mds_couple_id: 1234
        mds_key_version: 5
        mds_key_uuid: 11111111-8888-1111-1111-111111111111
        metadata: '{"encryption": {"meta": "Gandalf"}}'
        """
        Then inflight has part "0" for object "Gandalf":
        """
        mds_couple_id: 1234
        mds_key_version: 5
        mds_key_uuid: 11111111-8888-1111-1111-111111111111
        metadata: '{"encryption": {"meta": "Gandalf"}}'
        """
        When we complete inflight upload for object "Gandalf" w/o errors
        Then inflight has parts count of "0" for object "Gandalf"
         And object "Gandalf" has fields:
         """
         metadata: '{"encryption": {"meta": "Gandalf"}}'
         mds_couple_id: 1234
         mds_key_version: 5
         mds_key_uuid: 11111111-8888-1111-1111-111111111111
         parts_count: 0
         """
         And we have no errors in counters
        When we list all in the storage delete queue
        Then we get the following deleted object(s) as a result:
          """
          - name: Gandalf
            data_md5: 11111111-1111-1111-1111-111111111111
            mds_couple_id: 111
            mds_key_version: 1
            mds_key_uuid: 11111111-0000-1111-1111-111111111111
          """
