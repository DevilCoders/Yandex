Feature: Listing object versions

  Background: Adding needed objects
    Given buckets owner account "1"
     And a bucket with name "Tolkien"
     And we enable bucket versioning
    When we add object "friends/legolas" with following attributes:
        """
        data_size: 1111
        """
     And we add object "friends/aragorn" with following attributes:
        """
        data_size: 2222
        """
     And we add object "friends/gimli" with following attributes:
        """
        data_size: 3333
        """
     And we add object "TheLordOfTheRings/torin" with following attributes:
        """
        data_size: 4444
        """
     And we add delete marker with name "TheLordOfTheRings/torin"
     And we add object "TheLordOfTheRings/durin" with following attributes:
        """
        data_size: 5555
        """
     And we add delete marker with name "TheLordOfTheRings/durin"
     And we add object "HobbitThereAndBack/dain" with following attributes:
        """
        data_size: 6666
        """
     And we add object "foes/sauron" with following attributes:
        """
        data_size: 1111
        """
     And we add object "friends_other/gnomes" with following attributes:
        """
        data_size: 1111
        """

  Scenario: Listing versions with prefix
    When we list all versions in a bucket with prefix "TheLordOfTheRings", delimiter "NULL" and start_after "NULL"
    Then we get following list items:
        """
        - name: TheLordOfTheRings/durin
          is_prefix: false
        - name: TheLordOfTheRings/torin
          is_prefix: false
        """
    When we list all objects in a bucket with prefix "TheLordOfTheRings", delimiter "NULL" and start_after "NULL"
    Then we get following objects:
        """
        """

  Scenario: Listing versions with prefix and start_after
    When we list all versions in a bucket with prefix "friends", delimiter "NULL" and start_after "friends/gendalf"
    Then we get following list items:
        """
        - name: friends/gimli
          is_prefix: false

        - name: friends/legolas
          is_prefix: false

        - name: friends_other/gnomes
          is_prefix: false
        """

  Scenario: Listing versions with prefix and delimiter
    When we list all versions in a bucket with prefix "NULL", delimiter "/" and start_after "NULL"
    Then we get following list items:
        """
        - name: HobbitThereAndBack/
          is_prefix: true

        - name: TheLordOfTheRings/
          is_prefix: true

        - name: foes/
          is_prefix: true

        - name: friends/
          is_prefix: true

        - name: friends_other/
          is_prefix: true
        """

  Scenario: Listing versions with delimiter and start_after
    When we list all versions in a bucket with prefix "NULL", delimiter "/" and start_after "TheLordOfTheRings/"
    Then we get following objects:
        """
        - name: foes/
          is_prefix: true

        - name: friends/
          is_prefix: true

        - name: friends_other/
          is_prefix: true
        """

  Scenario: Listing versions with prefix, delimiter and start_after
    When we list all versions in a bucket with prefix "f", delimiter "/" and start_after "foes/"
    Then we get following objects:
        """
        - name: friends/
          is_prefix: true

        - name: friends_other/
          is_prefix: true
        """
     And we have no errors in counters
