Feature: MongoDB backup scheduler plan works

  Scenario: On empty database it works
    Given databases timezone set to "Europe/Moscow"
    When I execute mdb-backup-scheduler for ctype "mongodb_cluster" to plan
    Then in metadb there are "0" backups

  Scenario: On unsupported databases it works
    When we add default feature flag "MDB_REDIS_62"
    When I create "1" redis clusters
    When I create "1" hadoop clusters
    And I set "redis_01" cluster backup window at "+1m" from now
    And I execute mdb-backup-scheduler for ctype "mongodb_cluster" to plan with config
    """
    schedule_config:
      past_interval: 1h
      future_interval: 30m
    """
    Then in metadb there are "0" backups

  Scenario: On supported databases and disabled backup-service it does not works
    When I create "1" mongodb clusters
    And I disable backup service for all existed clusters via cli
    And I set "mongodb_01" cluster backup window at "+1h" from now
    And I execute mdb-backup-scheduler for ctype "mongodb_cluster" to plan with config
    """
    schedule_config:
      past_interval: 2h
      future_interval: 2h
    """
    Then in metadb there are "0" backups

  Scenario: On supported databases it works
    Given databases timezone set to "UTC"
    When I create "2" mongodb clusters
    And I set "mongodb_01" cluster backup window at "+5h" from now
    And I set "mongodb_02" cluster backup window at "-5h" from now
    And I execute mdb-backup-scheduler for ctype "mongodb_cluster" to plan with config
    """
    schedule_config:
      past_interval: 2h
      future_interval: 2h
    """
    Then in metadb there are "0" backups
    When I set "mongodb_01" cluster backup window at "+1h" from now
    And I execute mdb-backup-scheduler for ctype "mongodb_cluster" to plan with config
    """
    schedule_config:
      past_interval: 2h
      future_interval: 2h
    """
    Then in metadb there are "1" backups
    When I execute query
    """
    SELECT b.status as backup_status, c.name cluster_name, sc.name as subcluster_name, sh.name as shard_name
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "1" rows matches
    """
      - backup_status: PLANNED
        cluster_name: mongodb_01
        subcluster_name: mongod_subcluster
        shard_name: rs01
    """
    When I set "mongodb_02" cluster backup window at "-1h" from now
    And I execute mdb-backup-scheduler for ctype "mongodb_cluster" to plan with config
    """
    schedule_config:
      past_interval: 2h
      future_interval: 2h
    """
    Then in metadb there are "2" backups
    When I execute query
    """
    SELECT b.status as backup_status, c.name cluster_name, sc.name as subcluster_name, sh.name as shard_name
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "2" rows matches
    """
      - backup_status: PLANNED
        cluster_name: mongodb_01
        subcluster_name: mongod_subcluster
        shard_name: rs01
      - backup_status: PLANNED
        cluster_name: mongodb_02
        subcluster_name: mongod_subcluster
        shard_name: rs01
    """

  Scenario: On second run it works properly
    When I create "2" mongodb clusters
    And I set "mongodb_01" cluster backup window at "+1m" from now
    And I set "mongodb_02" cluster backup window at "-1m" from now
    And I execute mdb-backup-scheduler for ctype "mongodb_cluster" to plan with config
    """
    schedule_config:
      past_interval: 1h
      future_interval: 1h
    """
    Then in metadb there are "2" backups
    When I execute mdb-backup-scheduler for ctype "mongodb_cluster" to plan with config
    """
    schedule_config:
      past_interval: 1h
      future_interval: 1h
    """
    Then in metadb there are "2" backups

  Scenario: On sharded mongodb it works properly
    When I create "1" mongodb clusters
    And I set "mongodb_01" cluster backup window at "-10m" from now
    And I execute mdb-backup-scheduler for ctype "mongodb_cluster" to plan
    And I enable sharding on "mongodb_01" mongodb cluster
    And I execute mdb-backup-scheduler for ctype "mongodb_cluster" to plan
    Then in metadb there are "2" backups
    When I add shard "shard02" to "mongodb_01" cluster
    And I add shard "shard03" to "mongodb_01" cluster
    And I execute mdb-backup-scheduler for ctype "mongodb_cluster" to plan
    Then in metadb there are "4" backups
    When I execute query
    """
    SELECT b.status as backup_status, c.name cluster_name, sc.name as subcluster_name, sh.name as shard_name
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "4" rows matches
    """
      - backup_status: PLANNED
        cluster_name: mongodb_01
        subcluster_name: mongod_subcluster
        shard_name: rs01
      - backup_status: PLANNED
        cluster_name: mongodb_01
        subcluster_name: mongod_subcluster
        shard_name: shard02
      - backup_status: PLANNED
        cluster_name: mongodb_01
        subcluster_name: mongod_subcluster
        shard_name: shard02
      - backup_status: PLANNED
        cluster_name: mongodb_01
        subcluster_name: mongocfg_subcluster
        shard_name:
    """

  Scenario: On second run after moving backup window it works properly
    When I create "1" mongodb clusters
    And I set "mongodb_01" cluster backup window at "-10m" from now
    And I execute mdb-backup-scheduler for ctype "mongodb_cluster" to plan
    Then in metadb there are "1" backups
    When I set "mongodb_01" cluster backup window at "+10m" from now
    And I execute mdb-backup-scheduler for ctype "mongodb_cluster" to plan with config
    """
    schedule_config:
      past_interval: 1h
      future_interval: 1h
    """
    Then in metadb there are "1" backups

  Scenario Outline: With different timezones it works
    Given databases timezone set to "<timezone>"
    When I create "1" mongodb clusters
    And I set "mongodb_01" cluster backup window at "+1h" from now
    And I execute mdb-backup-scheduler for ctype "mongodb_cluster" to plan with config
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
