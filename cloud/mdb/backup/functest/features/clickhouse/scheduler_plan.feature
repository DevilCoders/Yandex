Feature: Clickhouse backup scheduler plan works

  Scenario: On supported databases and disabled backup-service it does not works
    When I create "1" clickhouse clusters
    And I disable backup service for all existed clusters via cli
    And I set "clickhouse_01" cluster backup window at "+1h" from now
    And I execute mdb-backup-scheduler for ctype "clickhouse_cluster" to plan with config
    """
    schedule_config:
      past_interval: 2h
      future_interval: 2h
    """
    Then in metadb there are "0" backups

  Scenario: On supported databases it works
    Given databases timezone set to "UTC"
    When I create "2" clickhouse clusters
    And I set "clickhouse_01" cluster backup window at "+5h" from now
    And I set "clickhouse_02" cluster backup window at "-5h" from now
    And I enable backup service for all existed clusters via cli
    And I execute mdb-backup-scheduler for ctype "clickhouse_cluster" to plan with config
    """
    schedule_config:
      past_interval: 2h
      future_interval: 2h
    """
    Then in metadb there are "0" backups
    When I set "clickhouse_01" cluster backup window at "+1h" from now
    And I execute mdb-backup-scheduler for ctype "clickhouse_cluster" to plan with config
    """
    schedule_config:
      past_interval: 2h
      future_interval: 2h
    """
    Then in metadb there are "1" backups
    When I execute query
    """
    SELECT b.status as backup_status, c.name cluster_name
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "1" rows matches
    """
      - backup_status: PLANNED
        cluster_name: clickhouse_01
    """
    When I set "clickhouse_02" cluster backup window at "-1h" from now
    And I execute mdb-backup-scheduler for ctype "clickhouse_cluster" to plan with config
    """
    schedule_config:
      past_interval: 2h
      future_interval: 2h
    """
    Then in metadb there are "2" backups
    When I execute query
    """
    SELECT b.status as backup_status, c.name cluster_name
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "2" rows matches
    """
      - backup_status: PLANNED
        cluster_name: clickhouse_01
      - backup_status: PLANNED
        cluster_name: clickhouse_02
    """

  Scenario: On second run it works properly
    When I create "2" clickhouse clusters
    And I set "clickhouse_01" cluster backup window at "+1m" from now
    And I set "clickhouse_02" cluster backup window at "-1m" from now
    And I enable backup service for all existed clusters via cli
    And I execute mdb-backup-scheduler for ctype "clickhouse_cluster" to plan with config
    """
    schedule_config:
      past_interval: 1h
      future_interval: 1h
    """
    Then in metadb there are "2" backups
    When I execute mdb-backup-scheduler for ctype "clickhouse_cluster" to plan with config
    """
    schedule_config:
      past_interval: 1h
      future_interval: 1h
    """
    Then in metadb there are "2" backups

  Scenario: On second run after moving backup window it works properly
    When I create "1" clickhouse clusters
    And I enable backup service for all existed clusters via cli
    And I set "clickhouse_01" cluster backup window at "-10m" from now
    And I execute mdb-backup-scheduler for ctype "clickhouse_cluster" to plan
    Then in metadb there are "1" backups
    When I set "clickhouse_01" cluster backup window at "+10m" from now
    And I execute mdb-backup-scheduler for ctype "clickhouse_cluster" to plan with config
    """
    schedule_config:
      past_interval: 1h
      future_interval: 1h
    """
    Then in metadb there are "1" backups

  Scenario Outline: With different timezones it works
    Given databases timezone set to "<timezone>"
    When I create "1" clickhouse clusters
    And I enable backup service for all existed clusters via cli
    And I set "clickhouse_01" cluster backup window at "+1h" from now
    And I execute mdb-backup-scheduler for ctype "clickhouse_cluster" to plan with config
    """
    schedule_config:
      past_interval: 2h
      future_interval: 2h
    """
    Then in metadb there are "1" backups
    Examples:
    | timezone |
    | UTC |
    | Europe/Moscow |
    | Asia/Anadyr |
    | Etc/GMT+3 |
    | Etc/GMT+12 |
    | Etc/GMT-12 |
    | Etc/GMT-14 |
