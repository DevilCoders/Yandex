@imp
Feature: katan imp works

  Background: Empty databases
    Given empty databases

  Scenario: On empty database it works
    When I execute mdb-katan-imp
    Then it succeeds

  Scenario: Imp imports clusters and their changes
    When I create "2" postgresql clusters
     And I execute mdb-katan-imp
    Then in katandb there are "2" clusters
     And in katandb there are "6" hosts
    When I delete one postgresql cluster
     And I execute mdb-katan-imp
    Then in katandb there is one cluster
     And in katandb there are "3" hosts
    When I create host in postgresql cluster
     And I execute mdb-katan-imp
    Then in katandb there are "4" hosts

  @MDB-7532
  Scenario: Imp doesn't import unmanaged clusters
    When I create hadoop cluster
     And I execute mdb-katan-imp
    Then it succeeds
     But in katandb there are "0" clusters

  Scenario: Imp update cluster tags when their version changed
    When I create "1" postgresql clusters
     And I execute mdb-katan-imp
    Then it succeeds
    Then in katandb there is one cluster with latest tags version
    When I set tags it's tags version to 1
     And I execute mdb-katan-imp
    Then in katandb there is one cluster with latest tags version
