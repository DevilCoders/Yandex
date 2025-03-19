Feature: MySQL backup import works

  Background: Empty databases
    Given databases timezone set to "UTC"
    When I create "1" mysql clusters
    And I execute mdb-backup-cli for cluster "mysql_01" to "enable backup-service"

  @import_mysql
  Scenario: Import mysql backups works
	Given s3 responses sequence
	"""
	{
		"Responses": [
			{
				"ListObjects": {
					"Contents": [
						{
							"Key": "wal-g/cid1/507/basebackups_005/stream_19700101T000030Z_backup_stop_sentinel.json",
							"LastModified": "1970-01-01T03:00:31.403547+03:00"
						},
						{
							"Key": "wal-g/cid1/507/basebackups_005/stream_19700101T000030Z/stream.br",
							"LastModified": "1970-01-01T03:00:31.403547+03:00"
						}
					]
				}
			},
			{
				"GetObject": {
					"Body": {
						"BinLogStart": "mysql-bin-log-sas-lolkek-db-yandex-net.000003",
						"BinLogEnd": "mysql-bin-log-sas-lolkek-db-yandex-net.000004",
						"StartLocalTime": "1970-01-01T03:00:30.403547Z",
						"StopLocalTime": "1970-01-01T03:00:31.403547Z",
						"Hostname": "sas-lolkek.db.yandex.net",
						"UncompressedSize": 31337,
						"IsPermanent": true,
						"CompressedSize": 1337
					}
				}
			}
		]
	}
	"""
	When I execute mdb-backup-cli for cluster "mysql_01" to "import s3 backups" and got
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
        initiator:     USER
        method:        FULL
        shard_id:      NULL
        subcid:        subcid1
        created_at:    1970-01-01T03:00:30.403547+00:00
        delayed_until: 1970-01-01T03:00:30.403547+00:00
        finished_at:   1970-01-01T03:00:31.403547+00:00
        started_at:    1970-01-01T03:00:30.403547+00:00
        updated_at:    1970-01-01T03:00:31.403547+00:00
    """
    And I list backups for "mysql_01" cluster and got
    """
    {
        "backups": [
            {
                "createdAt":        "1970-01-01T03:00:31.403547+00:00",
                "folderId":         "folder1",
                "id":               "cid1:backup_id1",
                "size":             1337,
                "sourceClusterId":  "cid1",
                "startedAt":        "1970-01-01T03:00:30.403547+00:00",
                "type":             "MANUAL",
                "sourceShardNames": null
            }
        ]
    }
    """

  @secondrun
  Scenario: Second run does not handle imported mysql backups
    Given s3 responses sequence
	"""
	{
		"Responses": [
			{
				"ListObjects": {
					"Contents": [
						{
							"Key": "wal-g/cid1/507/basebackups_005/stream_19700101T000030Z_backup_stop_sentinel.json",
							"LastModified": "1970-01-01T03:00:31.403547+03:00"
						},
						{
							"Key": "wal-g/cid1/507/basebackups_005/stream_19700101T000030Z/stream.br",
							"LastModified": "1970-01-01T03:00:31.403547+03:00"
						}
					]
				}
			},
			{
				"GetObject": {
					"Body": {
						"BinLogStart": "mysql-bin-log-sas-lolkek-db-yandex-net.000003",
						"BinLogEnd": "mysql-bin-log-sas-lolkek-db-yandex-net.000004",
						"StartLocalTime": "1970-01-01T03:00:30.403547Z",
						"StopLocalTime": "1970-01-01T03:00:31.403547Z",
						"Hostname": "sas-lolkek.db.yandex.net",
						"UncompressedSize": 31337,
						"IsPermanent": true,
						"CompressedSize": 1337
					}
				}
			}
		]
	}
	"""
    When I execute mdb-backup-cli for cluster "mysql_01" to "import s3 backups" and got
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
        initiator:     USER
        method:        FULL
        subcid:        subcid1
        shard_id:      NULL
        created_at:    1970-01-01T03:00:30.403547+00:00
        delayed_until: 1970-01-01T03:00:30.403547+00:00
        started_at:    1970-01-01T03:00:30.403547+00:00
        finished_at:   1970-01-01T03:00:31.403547+00:00
        updated_at:    1970-01-01T03:00:31.403547+00:00
    """
    And s3 responses sequence
    """
	{
		"Responses": [
			{
				"ListObjects": {
						"Contents": [
							{
								"Key": "wal-g/cid1/507/basebackups_005/stream_19700101T000030Z_backup_stop_sentinel.json",
								"LastModified": "1970-01-01T03:00:31.403547+03:00"
							},
							{
								"Key": "wal-g/cid1/507/basebackups_005/stream_19700101T000030Z/stream.br",
								"LastModified": "1970-01-01T03:00:31.403547+03:00"
							},
							{
								"Key": "wal-g/cid1/507/basebackups_005/stream_19700102T000030Z_backup_stop_sentinel.json",
								"LastModified": "1971-01-02T03:00:31.403547+03:00"
							},
							{
								"Key": "wal-g/cid1/507/basebackups_005/stream_19700102T000030Z/stream.br",
								"LastModified": "1971-01-02T03:00:31.403547+03:00"
							}
						]
					}
			},
			{
				"GetObject": {
					"Body": {
						"BinLogStart": "mysql-bin-log-sas-lolkek-db-yandex-net.000003",
						"BinLogEnd": "mysql-bin-log-sas-lolkek-db-yandex-net.000004",
						"StartLocalTime": "1970-01-01T03:00:30.403547Z",
						"StopLocalTime": "1970-01-01T03:00:31.403547Z",
						"Hostname": "sas-lolkek.db.yandex.net",
						"UncompressedSize": 31337,
						"CompressedSize": 1337
					}
				}
			},
			{
				"GetObject": {
					"Body": {
						"BinLogStart": "mysql-bin-log-sas-lolkek-db-yandex-net.000004",
						"BinLogEnd": "mysql-bin-log-sas-lolkek-db-yandex-net.000005",
						"StartLocalTime": "1971-01-01T03:00:30.403547Z",
						"StopLocalTime": "1971-01-01T03:00:31.403547Z",
						"Hostname": "sas-lolkek.db.yandex.net",
						"UncompressedSize": 31338,
						"CompressedSize": 1338
					}
				}
			}
		]
	}
    """
    When I execute mdb-backup-cli for cluster "mysql_01" to "import s3 backups" and got
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
        initiator:     USER
        method:        FULL
        subcid:        subcid1
        shard_id:      NULL
        created_at:    1970-01-01T03:00:30.403547+00:00
        delayed_until: 1970-01-01T03:00:30.403547+00:00
        started_at:    1970-01-01T03:00:30.403547+00:00
        finished_at:   1970-01-01T03:00:31.403547+00:00
        updated_at:    1970-01-01T03:00:31.403547+00:00
      - backup_id:     backup_id2
        status:        DONE
        initiator:     SCHEDULE
        method:        FULL
        subcid:        subcid1
        shard_id:      NULL
        created_at:    1971-01-01T03:00:30.403547+00:00
        delayed_until: 1971-01-01T03:00:30.403547+00:00
        started_at:    1971-01-01T03:00:30.403547+00:00
        finished_at:   1971-01-01T03:00:31.403547+00:00
        updated_at:    1971-01-01T03:00:31.403547+00:00
    """
    And I list backups for "mysql_01" cluster and got
    """
    {
		"backups": [
			{
				"createdAt":        "1971-01-01T03:00:31.403547+00:00",
				"folderId":         "folder1",
				"id":               "cid1:backup_id2",
				"size":             1338,
				"sourceClusterId":  "cid1",
				"sourceShardNames": null,
				"startedAt":        "1971-01-01T03:00:30.403547+00:00",
				"type":             "AUTOMATED"
			},
			{
				"createdAt":        "1970-01-01T03:00:31.403547+00:00",
				"folderId":         "folder1",
				"id":               "cid1:backup_id1",
				"size":             1337,
				"sourceClusterId":  "cid1",
				"sourceShardNames": null,
				"startedAt":        "1970-01-01T03:00:30.403547+00:00",
				"type":             "MANUAL"
			}
		]
	}
    """

  @backup_service_usage
  Scenario: CLI disables and enables backup service usage for MySQL cluster
    When I execute mdb-backup-cli for cluster "mysql_01" to "disable backup-service"
    Then backup service is "disabled" for "mysql_01" cluster
    When I execute mdb-backup-cli for cluster "mysql_01" to "enable backup-service"
    Then backup service is "enabled" for "mysql_01" cluster

  @roller
  Scenario: CLI rolls metadata on MySQL cluster hosts
    Given all deploy shipments get status is "DONE"
    When I execute mdb-backup-cli for cluster "mysql_01" to "roll metadata"
