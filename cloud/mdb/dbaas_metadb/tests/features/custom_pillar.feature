Feature: Custom pillar by host
  Background: cloud and folder
    Given default database
    And cloud with default quota
    And folder
    And "flavor1" flavor

  Scenario: Add 3 hosts and check cluster_nodes for postgres
    Given cluster with
    """
    type: postgresql_cluster
    """
    And "postgresql_cluster" subcluster
    And new fqdn as "fqdn"
    And new fqdn as "fqdn1"
    And new fqdn as "fqdn2"
    And "fqdn" host
    And "fqdn1" host
    And "fqdn2" host
    When I execute query
    """
    SELECT jsonb_array_elements_text(get_pg_cluster_nodes -> 'ha') AS x
      FROM code.get_pg_cluster_nodes(:fqdn);
    """
    Then it returns "3" rows matches
    """
    - x: {{ fqdn }}
    - x: {{ fqdn1 }}
    - x: {{ fqdn2 }}
    """

    When I successfully execute query
    """
    INSERT into dbaas.pillar (fqdn, value) VALUES
    (:fqdn2,
     concat('{"data": {"pgsync": {"replication_source": "', :fqdn1, '"}}}')::jsonb)
    """
    And I execute query
    """
    SELECT jsonb_array_elements_text(get_pg_cluster_nodes -> 'ha') AS x
      FROM code.get_pg_cluster_nodes(:fqdn);
    """
    Then it returns "2" rows matches
    """
    - x: {{ fqdn }}
    - x: {{ fqdn1 }}
    """
    When I execute query
    """
    SELECT jsonb_array_elements_text(get_custom_pillar_by_host -> 'data' -> 'cluster_nodes' -> 'cascade_replicas') AS x
      FROM code.get_custom_pillar_by_host(:fqdn1);
    """
    Then it returns one row matches
    """
    x: {{ fqdn2 }}
    """

  Scenario: Add 3 hosts and check cluster_nodes for mysql
    Given cluster with
    """
    env: qa
    type: mysql_cluster
    name: pillar test
    """
    And "mysql_cluster" subcluster
    And new fqdn as "fqdn"
    And new fqdn as "fqdn1"
    And new fqdn as "fqdn2"
    And "fqdn" host
    And "fqdn1" host
    And "fqdn2" host
    When I execute query
    """
    SELECT jsonb_array_elements_text(get_mysql_cluster_nodes -> 'ha') AS x
      FROM code.get_mysql_cluster_nodes(:fqdn);
    """
    Then it returns "3" rows matches
    """
    - x: {{ fqdn }}
    - x: {{ fqdn1 }}
    - x: {{ fqdn2 }}
    """

    When I successfully execute query
    """
    INSERT into dbaas.pillar (fqdn, value) VALUES
    (:fqdn2,
     concat('{"data": {"mysql": {"replication_source": "', :fqdn1, '"}}}')::jsonb)
    """
    And I execute query
    """
    SELECT jsonb_array_elements_text(get_mysql_cluster_nodes -> 'ha') AS x
      FROM code.get_mysql_cluster_nodes(:fqdn);
    """
    Then it returns "2" rows matches
    """
    - x: {{ fqdn }}
    - x: {{ fqdn1 }}
    """
    When I execute query
    """
    SELECT jsonb_array_elements_text(get_custom_pillar_by_host -> 'data' -> 'cluster_nodes' -> 'cascade_replicas') AS x
      FROM code.get_custom_pillar_by_host(:fqdn1);
    """
    Then it returns one row matches
    """
    x: {{ fqdn2 }}
    """

  Scenario Outline: Check custom pillar for <type> cluster
    Given cluster with
    """
    env: qa
    type: <type>
    name: pillar test
    """
    And "<subcluster_type>" subcluster
    And new fqdn
    And host

    When I execute query
    """
    select code.get_custom_pillar_by_host(:fqdn)
    """
    Then it returns
    """
    [[{}]]
    """
    Examples:
    | type               | subcluster_type    |
    | redis_cluster      | redis_cluster      |
    | mongodb_cluster    | mongodb_cluster    |
    | clickhouse_cluster | clickhouse_cluster |
