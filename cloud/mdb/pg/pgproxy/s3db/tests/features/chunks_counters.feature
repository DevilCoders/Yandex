Feature: Chunks counters

  Background: Set buckets owner account
    Given empty DB
    Given buckets owner account "1"

  Scenario: Aggregated chunks counters works
    Given a bucket with name "JoanneRowling"
    When we add object "friends/NevilleLongbottom" with following attributes:
        """
        data_size: 1111
        """
    Then bucket "JoanneRowling" has "1" object(s) of size "1111"
     And bucket "JoanneRowling" has "0" multipart object(s) of size "0"
     And bucket "JoanneRowling" has "0" object part(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "0" object(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "0" object part(s) of size "0"
     And bucket "JoanneRowling" has "0" deleted objects of size "0" in counters queue
     And bucket "JoanneRowling" has "0" active multipart updload(s) in counters queue
    When we add object "friends/RonaldWeasley" with following attributes:
        """
        data_size: 2222
        """
    Then bucket "JoanneRowling" has "2" object(s) of size "3333"
     And bucket "JoanneRowling" has "0" multipart object(s) of size "0"
     And bucket "JoanneRowling" has "0" object part(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "0" object(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "0" object part(s) of size "0"
     And bucket "JoanneRowling" has "0" deleted objects of size "0" in counters queue
     And bucket "JoanneRowling" has "0" active multipart updload(s) in counters queue
    When we add object "friends/HermioneGranger" with following attributes:
        """
        data_size: 3333
        """
    Then bucket "JoanneRowling" has "3" object(s) of size "6666"
     And bucket "JoanneRowling" has "0" multipart object(s) of size "0"
     And bucket "JoanneRowling" has "0" object part(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "0" object(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "0" object part(s) of size "0"
     And bucket "JoanneRowling" has "0" deleted objects of size "0" in counters queue
     And bucket "JoanneRowling" has "0" active multipart updload(s) in counters queue
    When we add object "friends/NevilleLongbottom" with following attributes:
        """
        data_size: 4444
        """
    Then bucket "JoanneRowling" has "3" object(s) of size "9999"
     And bucket "JoanneRowling" has "0" multipart object(s) of size "0"
     And bucket "JoanneRowling" has "0" object part(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "1" object(s) of size "1111"
     And bucket "JoanneRowling" in delete queue has "0" object part(s) of size "0"
     And bucket "JoanneRowling" has "1" deleted objects of size "1111" in counters queue
     And bucket "JoanneRowling" has "0" active multipart updload(s) in counters queue
    When we drop object with name "friends/RonaldWeasley"
    Then bucket "JoanneRowling" has "2" object(s) of size "7777"
     And bucket "JoanneRowling" has "0" multipart object(s) of size "0"
     And bucket "JoanneRowling" has "0" object part(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "2" object(s) of size "3333"
     And bucket "JoanneRowling" in delete queue has "0" object part(s) of size "0"
     And bucket "JoanneRowling" has "2" deleted objects of size "3333" in counters queue
     And bucket "JoanneRowling" has "0" active multipart updload(s) in counters queue
    Given a multipart upload in bucket "JoanneRowling" for object "HarryPotter/DracoMalfoy"
    When we upload part for object "HarryPotter/DracoMalfoy":
        """
        part_id: 1
        data_size: 5555555
        """
    Then bucket "JoanneRowling" has "2" object(s) of size "7777"
     And bucket "JoanneRowling" has "0" multipart object(s) of size "0"
     And bucket "JoanneRowling" has "1" object part(s) of size "5555555"
     And bucket "JoanneRowling" in delete queue has "2" object(s) of size "3333"
     And bucket "JoanneRowling" in delete queue has "0" object part(s) of size "0"
     And bucket "JoanneRowling" has "2" deleted objects of size "3333" in counters queue
     And bucket "JoanneRowling" has "1" active multipart updload(s) in counters queue
    When we upload part for object "HarryPotter/DracoMalfoy":
        """
        part_id: 2
        data_size: 6666666
        """
    Then bucket "JoanneRowling" has "2" object(s) of size "7777"
     And bucket "JoanneRowling" has "0" multipart object(s) of size "0"
     And bucket "JoanneRowling" has "2" object part(s) of size "12222221"
     And bucket "JoanneRowling" in delete queue has "2" object(s) of size "3333"
     And bucket "JoanneRowling" in delete queue has "0" object part(s) of size "0"
    When we upload part for object "HarryPotter/DracoMalfoy":
        """
        part_id: 1
        data_size: 7777777
        """
    Then bucket "JoanneRowling" has "2" object(s) of size "7777"
     And bucket "JoanneRowling" has "0" multipart object(s) of size "0"
     And bucket "JoanneRowling" has "2" object part(s) of size "14444443"
     And bucket "JoanneRowling" in delete queue has "2" object(s) of size "3333"
     And bucket "JoanneRowling" in delete queue has "1" object part(s) of size "5555555"
     And bucket "JoanneRowling" has "3" deleted objects of size "5558888" in counters queue
     And bucket "JoanneRowling" has "1" active multipart updload(s) in counters queue
    When we complete the following multipart upload:
        """
        name: HarryPotter/DracoMalfoy
        data_md5: 77777777-7777-7777-7777-777777777777
        parts_data:
            - part_id: 1
              data_md5: 11111111-1111-1111-1111-111111111111

            - part_id: 2
              data_md5: 22222222-2222-2222-2222-222222222222
        """
    Then bucket "JoanneRowling" has "2" object(s) of size "7777"
     And bucket "JoanneRowling" has "1" multipart object(s) of size "14444443"
     And bucket "JoanneRowling" has "0" object part(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "2" object(s) of size "3333"
     And bucket "JoanneRowling" in delete queue has "1" object part(s) of size "5555555"
     And bucket "JoanneRowling" has "3" deleted objects of size "5558888" in counters queue
     And bucket "JoanneRowling" has "0" active multipart updload(s) in counters queue
    When we add object "HarryPotter/DracoMalfoy" with following attributes:
        """
        data_size: 8888
        """
    Then bucket "JoanneRowling" has "3" object(s) of size "16665"
     And bucket "JoanneRowling" has "0" multipart object(s) of size "0"
     And bucket "JoanneRowling" has "0" object part(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "2" object(s) of size "3333"
     And bucket "JoanneRowling" in delete queue has "3" object part(s) of size "19999998"
     And bucket "JoanneRowling" has "5" deleted objects of size "20003331" in counters queue
     And bucket "JoanneRowling" has "0" active multipart updload(s) in counters queue
    When we update chunks counters on "0" meta db
     And we update chunks counters on "1" meta db
    Then chunks counters for bucket "JoanneRowling" on meta and db are the same
    When we add multipart object "HarryPotter/AlbusDumbledore" with following part sizes:
        """
        [9999999, 1111111]
        """
    Then bucket "JoanneRowling" has "3" object(s) of size "16665"
     And bucket "JoanneRowling" has "1" multipart object(s) of size "11111110"
     And bucket "JoanneRowling" has "0" object part(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "2" object(s) of size "3333"
     And bucket "JoanneRowling" in delete queue has "3" object part(s) of size "19999998"
     And bucket "JoanneRowling" has "5" deleted objects of size "20003331" in counters queue
     And bucket "JoanneRowling" has "0" active multipart updload(s) in counters queue
    When we drop object with name "HarryPotter/AlbusDumbledore"
    Then bucket "JoanneRowling" has "3" object(s) of size "16665"
     And bucket "JoanneRowling" has "0" multipart object(s) of size "0"
     And bucket "JoanneRowling" has "0" object part(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "2" object(s) of size "3333"
     And bucket "JoanneRowling" in delete queue has "5" object part(s) of size "31111108"
     And bucket "JoanneRowling" has "7" deleted objects of size "31114441" in counters queue
     And bucket "JoanneRowling" has "0" active multipart updload(s) in counters queue
    Given a multipart upload in bucket "JoanneRowling" for object "HarryPotter/SeverusSnape"
    When we upload part for object "HarryPotter/SeverusSnape":
        """
        part_id: 1
        data_size: 2222222
        """
    When we upload part for object "HarryPotter/SeverusSnape":
        """
        part_id: 2
        data_size: 3333333
        """
    Then bucket "JoanneRowling" has "3" object(s) of size "16665"
     And bucket "JoanneRowling" has "0" multipart object(s) of size "0"
     And bucket "JoanneRowling" has "2" object part(s) of size "5555555"
     And bucket "JoanneRowling" in delete queue has "2" object(s) of size "3333"
     And bucket "JoanneRowling" in delete queue has "5" object part(s) of size "31111108"
     And bucket "JoanneRowling" has "7" deleted objects of size "31114441" in counters queue
     And bucket "JoanneRowling" has "1" active multipart updload(s) in counters queue
    When we abort multipart upload for object "HarryPotter/SeverusSnape"
    Then bucket "JoanneRowling" has "3" object(s) of size "16665"
     And bucket "JoanneRowling" has "0" multipart object(s) of size "0"
     And bucket "JoanneRowling" has "0" object part(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "2" object(s) of size "3333"
     And bucket "JoanneRowling" in delete queue has "7" object part(s) of size "36666663"
     And bucket "JoanneRowling" has "9" deleted objects of size "36669996" in counters queue
     And bucket "JoanneRowling" has "0" active multipart updload(s) in counters queue
    When we add multipart object "HarryPotter/RubeusHagrid" with following part sizes:
        """
        [44444444, 55555555]
        """
    Then bucket "JoanneRowling" has "3" object(s) of size "16665"
     And bucket "JoanneRowling" has "1" multipart object(s) of size "99999999"
     And bucket "JoanneRowling" has "0" object part(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "2" object(s) of size "3333"
     And bucket "JoanneRowling" in delete queue has "7" object part(s) of size "36666663"
     And bucket "JoanneRowling" has "9" deleted objects of size "36669996" in counters queue
     And bucket "JoanneRowling" has "0" active multipart updload(s) in counters queue
    Given a multipart upload in bucket "JoanneRowling" for object "HarryPotter/LordVoldemort"
    When we upload part for object "HarryPotter/LordVoldemort":
        """
        part_id: 1
        data_size: 55555555
        """
    Then bucket "JoanneRowling" has "3" object(s) of size "16665"
     And bucket "JoanneRowling" has "1" multipart object(s) of size "99999999"
     And bucket "JoanneRowling" has "1" object part(s) of size "55555555"
     And bucket "JoanneRowling" in delete queue has "2" object(s) of size "3333"
     And bucket "JoanneRowling" in delete queue has "7" object part(s) of size "36666663"
     And bucket "JoanneRowling" has "9" deleted objects of size "36669996" in counters queue
     And bucket "JoanneRowling" has "1" active multipart updload(s) in counters queue
    When we update chunk counters on "0" db
    When we update chunk counters on "1" db
    Then bucket "JoanneRowling" has "3" object(s) of size "16665"
     And bucket "JoanneRowling" has "1" multipart object(s) of size "99999999"
     And bucket "JoanneRowling" has "1" object part(s) of size "55555555"
     And bucket "JoanneRowling" has "0" object(s) in chunks counters queue
     And bucket "JoanneRowling" in delete queue has "2" object(s) of size "3333"
     And bucket "JoanneRowling" in delete queue has "7" object part(s) of size "36666663"
     And bucket "JoanneRowling" has "0" deleted objects of size "0" in counters queue
     And bucket "JoanneRowling" has "0" active multipart updload(s) in counters queue
    When we corrupt chunks counters for current bucket
    When we run repair script on "0" db
     And we run repair script on "1" db
    Then bucket "JoanneRowling" has "3" object(s) of size "16665"
     And bucket "JoanneRowling" has "1" multipart object(s) of size "99999999"
     And bucket "JoanneRowling" has "1" object part(s) of size "55555555"
     And bucket "JoanneRowling" has "0" object(s) in chunks counters queue
     And bucket "JoanneRowling" in delete queue has "2" object(s) of size "3333"
     And bucket "JoanneRowling" in delete queue has "7" object part(s) of size "36666663"
    When we update chunks counters on "0" meta db
     And we update chunks counters on "1" meta db
    Then chunks counters for bucket "JoanneRowling" on meta and db are the same
    When we add multipart object "friends/NevilleLongbottom" with following part sizes:
        """
        [66666666, 77777777]
        """
    Then bucket "JoanneRowling" has "2" object(s) of size "12221"
     And bucket "JoanneRowling" has "2" multipart object(s) of size "244444442"
     And bucket "JoanneRowling" has "1" object part(s) of size "55555555"
     And bucket "JoanneRowling" in delete queue has "3" object(s) of size "7777"
     And bucket "JoanneRowling" in delete queue has "7" object part(s) of size "36666663"
    When we add multipart object "friends/NevilleLongbottom" with following part sizes:
        """
        [88888888, 99999999]
        """
    Then bucket "JoanneRowling" has "2" object(s) of size "12221"
     And bucket "JoanneRowling" has "2" multipart object(s) of size "288888886"
     And bucket "JoanneRowling" has "1" object part(s) of size "55555555"
     And bucket "JoanneRowling" in delete queue has "3" object(s) of size "7777"
     And bucket "JoanneRowling" in delete queue has "9" object part(s) of size "181111106"
    When we update chunk counters on "0" db
    When we update chunk counters on "1" db
    Then bucket "JoanneRowling" has "2" object(s) of size "12221"
     And bucket "JoanneRowling" has "2" multipart object(s) of size "288888886"
     And bucket "JoanneRowling" has "1" object part(s) of size "55555555"
     And bucket "JoanneRowling" has "0" object(s) in chunks counters queue
     And bucket "JoanneRowling" in delete queue has "3" object(s) of size "7777"
     And bucket "JoanneRowling" in delete queue has "9" object part(s) of size "181111106"
    When we update chunks counters on "0" meta db
     And we update chunks counters on "1" meta db
    Then chunks counters for bucket "JoanneRowling" on meta and db are the same

  Scenario: Regress for bug from MDB-8270
    Given a bucket with name "JoanneRowling"
    When we add object "friends/NevilleLongbottom" with following attributes:
        """
        data_size: 1111
        data_md5: 11111111-1111-1111-1111-111111111111
        """
     And we update chunk counters on "0" db
     And we update chunk counters on "1" db
     And we add object "friends/RonaldWeasley" with following attributes:
        """
        data_size: 1111
        data_md5: 22222222-2222-2222-2222-222222222222
        """
     And we update chunk counters on "0" db
     And we update chunk counters on "1" db
    Then bucket "JoanneRowling" has "2" object(s) of size "2222"
     And chunks counters for bucket "JoanneRowling" contains rows
        """
          - simple_objects_count: 2
            simple_objects_size: 2222
        """

  Scenario: Enabled versioning counters
    Given a bucket with name "JoanneRowling"
     And we enable bucket versioning
    When we add object "friends/NevilleLongbottom" with following attributes:
        """
        data_size: 1000
        data_md5: 11111111-1111-1111-1111-111111111111
        """
     And we add multipart object "friends/NevilleLongbottom" with following part sizes:
        """
        [111111111, 222222222]
        """
     And we add delete marker with name "friends/NevilleLongbottom"
     And we update chunk counters on "0" db
     And we update chunk counters on "1" db
    Then bucket "JoanneRowling" has "2" object(s) of size "1025"
     And bucket "JoanneRowling" has "1" multipart object(s) of size "333333333"
     And bucket "JoanneRowling" has "0" object part(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "0" object(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "0" object part(s) of size "0"
    When we remove last version of object "friends/NevilleLongbottom"
    Then bucket "JoanneRowling" has "1" object(s) of size "1000"
     And bucket "JoanneRowling" has "1" multipart object(s) of size "333333333"
     And bucket "JoanneRowling" has "0" object part(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "0" object(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "0" object part(s) of size "0"
    When we remove last version of object "friends/NevilleLongbottom"
    Then bucket "JoanneRowling" has "1" object(s) of size "1000"
     And bucket "JoanneRowling" has "0" multipart object(s) of size "0"
     And bucket "JoanneRowling" has "0" object part(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "0" object(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "2" object part(s) of size "333333333"
    When we remove last version of object "friends/NevilleLongbottom"
    Then bucket "JoanneRowling" has "0" object(s) of size "0"
     And bucket "JoanneRowling" has "0" multipart object(s) of size "0"
     And bucket "JoanneRowling" has "0" object part(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "1" object(s) of size "1000"
     And bucket "JoanneRowling" in delete queue has "2" object part(s) of size "333333333"

  Scenario: Suspended versioning counters
    Given a bucket with name "JoanneRowling"
    When we add object "friends/NevilleLongbottom" with following attributes:
        """
        data_size: 1000
        """
    When we enable bucket versioning
     And we add object "friends/NevilleLongbottom" with following attributes:
        """
        data_size: 2000
        """
     And we update chunk counters on "0" db
     And we update chunk counters on "1" db
    Then bucket "JoanneRowling" has "2" object(s) of size "3000"
     And bucket "JoanneRowling" in delete queue has "0" object(s) of size "0"
    When we suspend bucket versioning
     And we add object "friends/NevilleLongbottom" with following attributes:
        """
        data_size: 4000
        """
     And we add delete marker with name "friends/NevilleLongbottom"
     And we update chunk counters on "0" db
     And we update chunk counters on "1" db
    Then bucket "JoanneRowling" has "2" object(s) of size "2025"
     And bucket "JoanneRowling" in delete queue has "2" object(s) of size "5000"
    When we remove last version of object "friends/NevilleLongbottom"
    Then bucket "JoanneRowling" has "1" object(s) of size "2000"
     And bucket "JoanneRowling" in delete queue has "2" object(s) of size "5000"
    When we remove last version of object "friends/NevilleLongbottom"
    Then bucket "JoanneRowling" has "0" object(s) of size "0"
     And bucket "JoanneRowling" in delete queue has "3" object(s) of size "7000"
    When we add delete marker with name "friends/NevilleLongbottom"
    Then bucket "JoanneRowling" has "1" object(s) of size "25"
     And bucket "JoanneRowling" in delete queue has "3" object(s) of size "7000"
