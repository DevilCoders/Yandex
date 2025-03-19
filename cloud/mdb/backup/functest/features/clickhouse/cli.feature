Feature: Clickhouse backup import works

  Background: Empty databases
    Given databases timezone set to "UTC"
    When I create "1" clickhouse clusters
    And I execute mdb-backup-cli for cluster "clickhouse_01" to "enable backup-service"

  @import_clickhouse
  Scenario: Import clickhouse backups works
    Given s3 responses sequence
    """
    {
      "Responses": [
        {
            "ListObjects": {
              "CommonPrefixes": [
                  {
                      "Prefix": "ch_backup/cid1/shard1"
                  }
              ]
            }
        },
        {
            "ListObjects": {
              "CommonPrefixes": [
                  {
                      "Prefix": "ch-backup/cid1/shard1/1"
                  }
              ]
            }
        },
        {
          "GetObject": {
            "Body": {
                "meta": {
                    "date_fmt": "%Y-%m-%d %H:%M:%S",
                    "start_time": "1970-01-01 00:00:10",
                    "end_time": "1970-01-01 00:00:11",
                    "labels": {
                        "shard_name": "shard1"
                    },
                    "bytes": 1024,
                    "state": "created"
                }
            }
          }
        }
      ]
    }
    """
    When I execute mdb-backup-cli for cluster "clickhouse_01" to "import s3 backups" and got
    """
    {"CompletedCreation": 0, "ExistsInMetadb": 0, "ExistsInStorage": 1, "ImportedIntoMetadb": 1, "SkippedDueToDryRun": 0, "SkippedDueToExistence": 0, "SkippedDueToUniqSchedDate": 0, "SkippedNoIncrementBase": 0}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status, b.initiator, b.method, b.subcid, b.shard_id, b.created_at, b.delayed_until, b.started_at, b.finished_at, b.updated_at
    FROM dbaas.backups b
    """
    Then it returns "1" rows matches
    """
      - backup_id:     backup_id1
        status:        DONE
        initiator:     SCHEDULE
        method:        FULL
        shard_id:      shard_id1
        subcid:        subcid1
        created_at:    1970-01-01T00:00:10+00:00
        delayed_until: 1970-01-01T00:00:10+00:00
        started_at:    1970-01-01T00:00:10+00:00
        finished_at:   1970-01-01T00:00:11+00:00
        updated_at:    1970-01-01T00:00:11+00:00
    """

  @secondrun
  Scenario: Second run does not handle imported clickhouse backups
    Given s3 responses sequence
    """
    {
      "Responses": [
        {
            "ListObjects": {
              "CommonPrefixes": [
                  {
                      "Prefix": "ch_backup/cid1/shard1"
                  }
              ]
            }
        },
        {
            "ListObjects": {
              "CommonPrefixes": [
                  {
                      "Prefix": "ch-backup/cid1/shard1/1"
                  }
              ]
            }
        },
        {
          "GetObject": {
            "Body": {
                "meta": {
                    "date_fmt": "%Y-%m-%d %H:%M:%S",
                    "start_time": "1970-01-01 00:00:10",
                    "end_time": "1970-01-01 00:00:11",
                    "labels": {
                        "shard_name": "shard1"
                    },
                    "bytes": 1024,
                    "state": "created"
                }
            }
          }
        }
      ]
    }
    """
    When I execute mdb-backup-cli for cluster "clickhouse_01" to "import s3 backups" and got
    """
    {"CompletedCreation": 0, "ExistsInMetadb": 0, "ExistsInStorage": 1, "ImportedIntoMetadb": 1, "SkippedDueToDryRun": 0, "SkippedDueToExistence": 0, "SkippedDueToUniqSchedDate": 0, "SkippedNoIncrementBase": 0}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status, b.initiator, b.method, b.subcid, b.shard_id, b.created_at, b.delayed_until, b.started_at, b.finished_at, b.updated_at
    FROM dbaas.backups b
    """
    Then it returns "1" rows matches
    """
      - backup_id:     backup_id1
        status:        DONE
        initiator:     SCHEDULE
        method:        FULL
        shard_id:      shard_id1
        subcid:        subcid1
        created_at:    1970-01-01T00:00:10+00:00
        delayed_until: 1970-01-01T00:00:10+00:00
        started_at:    1970-01-01T00:00:10+00:00
        finished_at:   1970-01-01T00:00:11+00:00
        updated_at:    1970-01-01T00:00:11+00:00
    """
    And s3 responses sequence
    """
    {
      "Responses": [
        {
            "ListObjects": {
              "CommonPrefixes": [
                  {
                      "Prefix": "ch_backup/cid1/shard1"
                  }
              ]
            }
        },
        {
            "ListObjects": {
              "CommonPrefixes": [
                  {
                      "Prefix": "ch-backup/cid1/shard1/1"
                  },
                  {
                      "Prefix": "ch-backup/cid1/shard1/2"
                  }
              ]
            }
        },
        {
          "GetObject": {
            "Body": {
                "meta": {
                    "date_fmt": "%Y-%m-%d %H:%M:%S",
                    "start_time": "1970-01-01 00:00:10",
                    "end_time": "1970-01-01 00:00:11",
                    "labels": {
                        "shard_name": "shard1"
                    },
                    "bytes": 1024,
                    "state": "created"
                }
            }
          }
        },
        {
          "GetObject": {
            "Body": {
                "meta": {
                    "date_fmt": "%Y-%m-%d %H:%M:%S",
                    "start_time": "1970-01-02 00:00:15",
                    "end_time": "1970-01-02 00:00:16",
                    "labels": {
                        "shard_name": "shard1"
                    },
                    "bytes": 1024,
                    "state": "created"
                }
            }
          }
        }
      ]
    }
    """
    When I execute mdb-backup-cli for cluster "clickhouse_01" to "import s3 backups" and got
    """
	  {"CompletedCreation": 0, "ExistsInMetadb": 1, "ExistsInStorage": 2, "ImportedIntoMetadb": 1, "SkippedDueToDryRun": 0, "SkippedDueToExistence": 1, "SkippedDueToUniqSchedDate": 0, "SkippedNoIncrementBase": 0}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status, b.initiator, b.method, b.subcid, b.shard_id, b.created_at, b.delayed_until, b.started_at, b.finished_at, b.updated_at
    FROM dbaas.backups b
    """
    Then it returns "2" rows matches
    """
      - backup_id:     backup_id1
        status:        DONE
        initiator:     SCHEDULE
        method:        FULL
        shard_id:      shard_id1
        subcid:        subcid1
        created_at:    1970-01-01T00:00:10+00:00
        delayed_until: 1970-01-01T00:00:10+00:00
        started_at:    1970-01-01T00:00:10+00:00
        finished_at:   1970-01-01T00:00:11+00:00
        updated_at:    1970-01-01T00:00:11+00:00
      - backup_id:     backup_id2
        status:        DONE
        initiator:     SCHEDULE
        method:        FULL
        shard_id:      shard_id1
        subcid:        subcid1
        created_at:    1970-01-02T00:00:15+00:00
        delayed_until: 1970-01-02T00:00:15+00:00
        started_at:    1970-01-02T00:00:15+00:00
        finished_at:   1970-01-02T00:00:16+00:00
        updated_at:    1970-01-02T00:00:16+00:00
    """

  @backup_service_usage
  Scenario: CLI disables and enables backup service usage for Clickhouse cluster
    When I execute mdb-backup-cli for cluster "clickhouse_01" to "disable backup-service"
    Then backup service is "disabled" for "clickhouse_01" cluster
    When I execute mdb-backup-cli for cluster "clickhouse_01" to "enable backup-service"
    Then backup service is "enabled" for "clickhouse_01" cluster

  @roller
  Scenario: CLI rolls metadata on Clickhouse cluster hosts
    Given all deploy shipments get status is "DONE"
    When I execute mdb-backup-cli for cluster "clickhouse_01" to "roll metadata"
