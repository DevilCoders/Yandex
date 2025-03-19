Feature: Listing objects in disabled versioning bucket

  Background: Adding needed objects
    Given buckets owner account "1"
    Given a bucket with name "Tolkien"
    When we add object "friends/legolas" with following attributes:
        """
        data_size: 1111
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
     And we add object "friends/aragorn" with following attributes:
        """
        data_size: 2222
        data_md5: 22222222-2222-2222-2222-222222222222
        mds_namespace: ns-2
        mds_couple_id: 222
        mds_key_version: 1
        mds_key_uuid: 22222222-2222-2222-2222-222222222222
        """
     And we add object "friends/gimli" with following attributes:
        """
        data_size: 3333
        data_md5: 33333333-3333-3333-3333-333333333333
        mds_namespace: ns-3
        mds_couple_id: 333
        mds_key_version: 1
        mds_key_uuid: 33333333-3333-3333-3333-333333333333
        """
     And we add object "TheLordOfTheRings/torin" with following attributes:
        """
        data_size: 4444
        data_md5: 44444444-4444-4444-4444-444444444444
        mds_namespace: ns-4
        mds_couple_id: 444
        mds_key_version: 1
        mds_key_uuid: 44444444-4444-4444-4444-444444444444
        """
     And we add object "TheLordOfTheRings/durin" with following attributes:
        """
        data_size: 5555
        data_md5: 55555555-5555-5555-5555-555555555555
        mds_namespace: ns-5
        mds_couple_id: 555
        mds_key_version: 1
        mds_key_uuid: 55555555-5555-5555-5555-555555555555
        """
     And we add object "HobbitThereAndBack/dain" with following attributes:
        """
        data_size: 6666
        data_md5: 66666666-6666-6666-6666-666666666666
        mds_namespace: ns-6
        mds_couple_id: 666
        mds_key_version: 1
        mds_key_uuid: 66666666-6666-6666-6666-666666666666
        """
     And we add object "foes/sauron" with following attributes:
        """
        data_size: 1111
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """
     And we add object "friends_other/gnomes" with following attributes:
        """
        data_size: 1111
        data_md5: 11111111-1111-1111-1111-111111111111
        mds_namespace: ns-1
        mds_couple_id: 111
        mds_key_version: 1
        mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """

  Scenario: Listing objects with prefix
    Given a bucket with name "Tolkien"
    When we list all objects in a bucket with prefix "TheLordOfTheRings", delimiter "NULL" and start_after "NULL"
    Then we get following objects:
        """
        - name: TheLordOfTheRings/durin
          data_size: 5555
          data_md5: 55555555-5555-5555-5555-555555555555
          mds_namespace: ns-5
          mds_couple_id: 555
          mds_key_version: 1
          mds_key_uuid: 55555555-5555-5555-5555-555555555555

        - name: TheLordOfTheRings/torin
          data_size: 4444
          data_md5: 44444444-4444-4444-4444-444444444444
          mds_namespace: ns-4
          mds_couple_id: 444
          mds_key_version: 1
          mds_key_uuid: 44444444-4444-4444-4444-444444444444
        """

  Scenario: Listing objects with prefix and start_after
    Given a bucket with name "Tolkien"
    When we list all objects in a bucket with prefix "friends", delimiter "NULL" and start_after "friends/gendalf"
    Then we get following objects:
        """
        - name: friends/gimli
          data_size: 3333
          data_md5: 33333333-3333-3333-3333-333333333333
          mds_namespace: ns-3
          mds_couple_id: 333
          mds_key_version: 1
          mds_key_uuid: 33333333-3333-3333-3333-333333333333

        - name: friends/legolas
          data_size: 1111
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns-1
          mds_couple_id: 111
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111

        - name: friends_other/gnomes
          data_size: 1111
          data_md5: 11111111-1111-1111-1111-111111111111
          mds_namespace: ns-1
          mds_couple_id: 111
          mds_key_version: 1
          mds_key_uuid: 11111111-1111-1111-1111-111111111111
        """

  Scenario: Listing objects with prefix and delimiter
    Given a bucket with name "Tolkien"
    When we list all objects in a bucket with prefix "NULL", delimiter "/" and start_after "NULL"
    Then we get following objects:
        """
        - name: HobbitThereAndBack/
          data_size:
          data_md5:
          mds_namespace:
          mds_couple_id:
          mds_key_version:
          mds_key_uuid:

        - name: TheLordOfTheRings/
          data_size:
          data_md5:
          mds_namespace:
          mds_couple_id:
          mds_key_version:
          mds_key_uuid:

        - name: foes/
          data_size:
          data_md5:
          mds_namespace:
          mds_couple_id:
          mds_key_version:
          mds_key_uuid:

        - name: friends/
          data_size:
          data_md5:
          mds_namespace:
          mds_couple_id:
          mds_key_version:
          mds_key_uuid:

        - name: friends_other/
          data_size:
          data_md5:
          mds_namespace:
          mds_couple_id:
          mds_key_version:
          mds_key_uuid:
        """

  Scenario: Listing objects with delimiter and start_after
    Given a bucket with name "Tolkien"
    When we list all objects in a bucket with prefix "NULL", delimiter "/" and start_after "TheLordOfTheRings/"
    Then we get following objects:
        """
        - name: foes/
          data_size:
          data_md5:
          mds_namespace:
          mds_couple_id:
          mds_key_version:
          mds_key_uuid:

        - name: friends/
          data_size:
          data_md5:
          mds_namespace:
          mds_couple_id:
          mds_key_version:
          mds_key_uuid:

        - name: friends_other/
          data_size:
          data_md5:
          mds_namespace:
          mds_couple_id:
          mds_key_version:
          mds_key_uuid:
        """

  Scenario: Listing objects with prefix, delimiter and start_after
    Given a bucket with name "Tolkien"
    When we list all objects in a bucket with prefix "f", delimiter "/" and start_after "foes/"
    Then we get following objects:
        """
        - name: friends/
          data_size:
          data_md5:
          mds_namespace:
          mds_couple_id:
          mds_key_version:
          mds_key_uuid:

        - name: friends_other/
          data_size:
          data_md5:
          mds_namespace:
          mds_couple_id:
          mds_key_version:
          mds_key_uuid:
        """
