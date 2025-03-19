Feature: Shards operations

  Background: All our operations and require cluster
    Given default database
    And cloud with default quota
    And folder
    And cluster with
      """
      type: clickhouse_cluster
      """
    And "clickhouse_cluster" subcluster

  Scenario: Add shard
    Given vars "shard_id"
    When I execute cluster change
      """
      SELECT * FROM code.add_shard(
      i_cid      => :cid,
      i_subcid   => :subcid,
      i_shard_id => :shard_id,
      i_name     => 'New shard',
      i_rev      => :rev
      )
      """
    Then it success
    When I execute query
      """
      SELECT name
      FROM dbaas.shards
      WHERE subcid = :subcid
      """
    Then it returns one row matches
      """
      name: New shard
      """

  Scenario: Delete shard
    Given shard
    When I execute cluster change
      """
      SELECT *
      FROM code.delete_shard(
      i_cid      => :cid,
      i_shard_id => :shard_id,
      i_rev      => :rev
      )
      """
    Then it success
    When I execute query
      """
      SELECT name
      FROM dbaas.shards
      WHERE subcid = :subcid
      """
    Then it returns
      """
      []
      """