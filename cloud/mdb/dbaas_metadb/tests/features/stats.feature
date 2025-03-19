Feature: Stats views

  Background: All our base stuff
    Given default database


  Scenario: Stats is empty from empty database
    Given fresh default database
    When I execute query
    """
    SELECT cpu_quota, cpu_used FROM stats.quota
    """
    Then It returns one row matches
    """
    cpu_quota: null
    cpu_used: null
    """

   Scenario: Stats created for new
    Given cloud with quota
    """
    cpu_quota: 10.0
    memory_quota: 200
    clusters_quota: 40
    ssd_space_quota: 4000
    hdd_space_quota: 8000
    """
    When I execute query
    """
    SELECT * FROM stats.quota
    """
    Then It returns one row matches
    """
    cpu_quota: 10.0
    cpu_used: 0
    memory_quota: 200
    memory_used: 0
    ssd_space_quota: 4000
    ssd_space_used: 0
    hdd_space_quota: 8000
    hdd_space_used: 0
    clusters_quota: 40
    clusters_used: 0
    """
    When I create new cloud with quota
    """
    cpu_quota: 10.0
    memory_quota: 200
    clusters_quota: 40
    ssd_space_quota: 4000
    hdd_space_quota: 8000
    """
    When I execute query
    """
    SELECT * FROM stats.quota
    """
    Then It returns one row matches
    """
    cpu_quota: 20.0
    cpu_used: 0
    memory_quota: 400
    memory_used: 0
    ssd_space_quota: 8000
    ssd_space_used: 0
    hdd_space_quota: 16000
    hdd_space_used: 0
    clusters_quota: 80
    clusters_used: 0
    """

   Scenario: per_cluster_type is empty when no clusters exists
    When I execute query
    """
    SELECT * FROM stats.per_cluster_type
    """
    Then It returns
    """
    []
    """

   Scenario: per_cluster_type view updates when add hosts
    Given "flavor4" flavor
    And cloud with default quota
    And folder
    And cluster with
    """
    type: mongodb_cluster
    """
    And "mongodb_cluster" subcluster
    And "5" new hosts
    And cluster with
    """
    type: clickhouse_cluster
    """
    And "clickhouse_cluster" subcluster
    And "flavor4" flavor
    And "2" new hosts
    And "zk" subcluster
    And "flavor2" flavor
    And "3" new hosts
    When I execute query
    """
    SELECT cluster_type,
           roles::text[],
           flavor_name,
           disk_type,
           hosts_count
      FROM stats.per_cluster_type
    """
    Then it returns "3" rows matches
    """
    - cluster_type: mongodb_cluster
      roles: [ 'mongodb_cluster' ]
      flavor_name: flavor4
      disk_type: disk_type1
      hosts_count: 5
    - cluster_type: clickhouse_cluster
      roles: [ 'clickhouse_cluster' ]
      flavor_name: flavor4
      disk_type: disk_type1
      hosts_count: 2
    - cluster_type: clickhouse_cluster
      roles: [ 'zk' ]
      flavor_name: flavor2
      disk_type: disk_type1
      hosts_count: 3
    """
    # delete clickhouse cluster
    Given task_id
    And "clickhouse_cluster_create" task
    And that task acquired by worker
    And that task finished by worker with result = true
    And task_id
    And "clickhouse_cluster_delete" task
    When I execute query
    """
    SELECT cluster_type,
           roles::text[],
           flavor_name,
           disk_type,
           hosts_count
      FROM stats.per_cluster_type
    """
    Then it returns one row matches
    """
    cluster_type: mongodb_cluster
    roles: [ 'mongodb_cluster' ]
    flavor_name: flavor4
    disk_type: disk_type1
    hosts_count: 5
    """
