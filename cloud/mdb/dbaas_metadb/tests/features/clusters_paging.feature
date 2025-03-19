Feature: Clusters paging

  Background: All our operations and require cluster
    Given default database
    And cloud with default quota
    And folder

  Scenario: Clusters paging
    Given "postgres" cluster with
    """
    name: Postgre is an open source relational database
    created_at: 1996-07-08
    """
    And "greenplum" cluster with
    """
    name: Greenplum is an open source data warehouse
    created_at: 2005-04-01
    """
    And "mongodb" cluster with
    """
    name: MongoDB is web scale
    created_at: 2009-02-11
    """
    And "redis" cluster with
    """
    name: Redis is an in-memory data structure store
    created_at: 2009-04-10
    """
    And "clickhouse" cluster with
    """
    name: ClickHouse is a free analytic DBMS for big data
    created_at: 2016-06-08
    """
    When I execute query
    """
    SELECT name
      FROM code.get_clusters(
        i_folder_id        => :folder_id,
        i_limit             => 2
    )
    """
    Then it returns "2" rows matches
    """
    - name: ClickHouse is a free analytic DBMS for big data
    - name: Greenplum is an open source data warehouse
    """
    When I execute query
    """
    SELECT name
      FROM code.get_clusters(
        i_folder_id        => :folder_id,
        i_limit             => 2,
        i_page_token_name   => :postgres_name
    )
    """
    Then it returns one row matches
    """
    name: Redis is an in-memory data structure store
    """
    When I execute query
    """
    SELECT name
      FROM code.get_clusters(
        i_folder_id        => :folder_id,
        i_limit             => 2,
        i_page_token_name   => :clickhouse_name
    )
    """
    Then it returns "2" rows matches
    """
    - name: Greenplum is an open source data warehouse
    - name: MongoDB is web scale
    """
