Feature: Subclusters operations

  Background: All our operations and require cluster
    Given default database
    And cloud with default quota
    And folder
    And cluster with
      """
      type: clickhouse_cluster
      """

  Scenario: Add subcluster
    Given vars "subcid"
    When I execute cluster change
      """
      SELECT * FROM code.add_subcluster(
      i_cid    => :cid,
      i_subcid => :subcid,
      i_name   => 'New subcluster',
      i_roles  => '{clickhouse_cluster}',
      i_rev    => :rev
      )
      """
    Then it success
    When I execute query
      """
      SELECT name
      FROM dbaas.subclusters
      WHERE cid = :cid
      """
    Then it returns one row matches
      """
      name: New subcluster
      """

  Scenario: Delete subcluster
    Given "clickhouse_cluster" subcluster
    When I execute cluster change
      """
      SELECT *
      FROM code.delete_subcluster(
      i_cid    => :cid,
      i_subcid => :subcid,
      i_rev    => :rev
      )
      """
    Then it success
    When I execute query
      """
      SELECT name
      FROM dbaas.subclusters
      WHERE cid = :cid
      """
    Then it returns
      """
      []
      """