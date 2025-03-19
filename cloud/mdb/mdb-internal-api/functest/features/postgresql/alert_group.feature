Feature: Alert Group PostgreSQL cluster

  Background:
    Given default headers

  @alert_group_creation
  Scenario: Alert Group creation works
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password"
       }],
       "hostSpecs": [{
           "zoneId": "myt"
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
       "description": "test cluster"
    }
    """
    And "worker_task_id1" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/alert-group" with data
    """
    {
		"monitoringFolderId": "test1",
		"alerts": [
			{
				"templateID": "Postgresql master alive",
				"criticalThreshold": 22,
				"notificationChannels": [ "ch1" ],
				"disabled": false
			},
			{
				"templateID": "Postgresql master walsize",
				"criticalThreshold": 33,
				"notificationChannels": [ "ch1" ],
				"disabled": false
			}
		]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create a Alert Group for PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateAlertsGroupClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.UpdateCluster" event with
    """
    {
        "details": {
            "alert_group_id": "alert_group1",
            "cluster_id": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1/alert-groups/alert_group1"
	Then we get response with status 200 and body contains
	"""
	{
		"alertGroupId":         "alert_group1",
		"monitoringFolderId":   "test1",
		"alerts": [
			{
				"notificationChannels": [ "ch1" ],
				"disabled": false,
				"criticalThreshold": 22,
				"warningThreshold": null,
				"templateID": "Postgresql master alive"
			},
			{
				"notificationChannels": [ "ch1" ],
				"disabled": false,
				"criticalThreshold": 33,
				"warningThreshold": null,
				"templateID": "Postgresql master walsize"
			}
		]
	}
	"""
    When we GET "/mdb/postgresql/1.0/clusters/cid1/alert-groups"
	Then we get response with status 200 and body contains
	"""
	{
		"alertGroups": [
			{
				"alertGroupId":         "alert_group1",
				"monitoringFolderId":   "test1",
				"alerts": [
					{
						"disabled": false,
						"notificationChannels": [ "ch1" ],
						"criticalThreshold": 22,
						"warningThreshold": null,
						"templateID": "Postgresql master alive"
					},
					{
						"disabled": false,
						"notificationChannels": [ "ch1" ],
						"criticalThreshold": 33,
						"warningThreshold": null,
						"templateID": "Postgresql master walsize"
					}
				]
			}
		]
	}
	"""

  @alert_group_deletion
  Scenario: Alert Group deletion works
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password"
       }],
       "hostSpecs": [{
           "zoneId": "myt"
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
       "description": "test cluster"
    }
    """
    And "worker_task_id1" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/alert-group" with data
    """
    {
		"monitoringFolderId": "test1",
		"alerts": [
			{
				"templateID": "Postgresql master alive",
				"criticalThreshold": 22,
				"notificationChannels": [
					"testch1",
					"testch2"
				]
			},
			{
				"templateID": "Postgresql master walsize",
				"criticalThreshold": 33,
				"notificationChannels": [
					"testch1",
					"testch2"
				]
			}
		]
    }
    """
    Then we get response with status 200
    And "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/alert-group" with data
    """
    {
		"monitoringFolderId": "test12",
		"alerts": [
			{
				"templateID": "Postgresql master alive",
				"criticalThreshold": 222,
				"notificationChannels": [ "testch12", "testch22" ]
			},
			{
				"templateID": "Postgresql master walsize",
				"criticalThreshold": 333,
				"notificationChannels": [ "testch12", "testch22" ]
			}
		]
    }
    """
    Then we get response with status 200
    And "worker_task_id3" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1/alert-groups"
	Then we get response with status 200 and body contains
	"""
	{
		"alertGroups": [
			{
				"alertGroupId":         "alert_group1",
				"monitoringFolderId":   "test1",
				"alerts": [
					{
						"disabled": false,
						"notificationChannels": [ "testch1", "testch2" ],
						"criticalThreshold": 22,
						"warningThreshold": null,
						"templateID": "Postgresql master alive"
					},
					{
						"disabled": false,
						"notificationChannels": [ "testch1", "testch2" ],
						"criticalThreshold": 33,
						"warningThreshold": null,
						"templateID": "Postgresql master walsize"
					}
				]
			},
			{
				"alertGroupId":         "alert_group2",
				"monitoringFolderId":   "test12",
				"alerts": [
					{
						"disabled": false,
						"notificationChannels": [ "testch12", "testch22" ],
						"criticalThreshold": 222,
						"warningThreshold": null,
						"templateID": "Postgresql master alive"
					},
					{
						"disabled": false,
						"notificationChannels": [ "testch12", "testch22" ],
						"criticalThreshold": 333,
						"warningThreshold": null,
						"templateID": "Postgresql master walsize"
					}
				]
			}
		]
	}
	"""
    When we DELETE "/mdb/postgresql/1.0/clusters/cid1/alert-group/alert_group1"
	Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete Alert Group for PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.DeleteAlertsGroupClusterMetadata",
            "clusterId": "cid1"
        }
    }
	"""
    When we GET "/mdb/postgresql/1.0/clusters/cid1/alert-groups"
	Then we get response with status 200 and body contains
	"""
	{
		"alertGroups": [
			{
				"alertGroupId":         "alert_group2",
				"monitoringFolderId":   "test12",
				"alerts": [
					{
						"disabled": false,
						"notificationChannels": [ "testch12", "testch22" ],
						"criticalThreshold": 222,
						"warningThreshold": null,
						"templateID": "Postgresql master alive"
					},
					{
						"disabled": false,
						"notificationChannels": [ "testch12", "testch22" ],
						"criticalThreshold": 333,
						"warningThreshold": null,
						"templateID": "Postgresql master walsize"
					}
				]
			}
		]
	}
	"""

  @alert_group_create_validation
  Scenario: Alert Group creation validation works
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password"
       }],
       "hostSpecs": [{
           "zoneId": "myt"
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
       "description": "test cluster"
    }
    """
    And "worker_task_id1" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/alert-group" with data
    """
    {
		"monitoringFolderId": "test1",
		"alerts": [
			{
				"templateID": "no such no such no such",
				"notificationChannels": [ "testch12", "testch22" ],
				"criticalThreshold": 22
			},
			{
				"templateID": "Postgresql master walsize",
				"notificationChannels": [ "testch12", "testch22" ],
				"criticalThreshold": 33
			}
		]
    }
    """
	Then we get response with status 404 and body contains
	"""
	{
		"code": 5,
		"message": "Alert metric 'no such no such no such' does not exists"
	}
	"""
    When we POST "/mdb/postgresql/1.0/clusters/cid1/alert-group" with data
    """
    {
		"monitoringFolderId": "test1",
		"alerts": [
			{
				"templateID": "Postgresql master walsize",
				"notificationChannels": [
				]
			}
		]
    }
    """
	Then we get response with status 422 and body contains
	"""
	{
		"code": 3,
		"message": "The request is invalid.\nalerts.0.notificationChannels: Shorter than minimum length 1."
	}
	"""
    When we POST "/mdb/postgresql/1.0/clusters/cid1/alert-group" with data
    """
    {
		"alerts": [
			{
				"templateID": "Postgresql master walsize",
				"notificationChannels": [
					"ch1"
				],
				"criticalThreshold": 33
			}
		]
    }
    """
	Then we get response with status 422 and body contains
	"""
	{
		"code": 3,
		"message": "The request is invalid.\nmonitoringFolderId: Missing data for required field."
	}
	"""

  @alert_update
  Scenario: Alert Group update works
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password"
       }],
       "hostSpecs": [{
           "zoneId": "myt"
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
       "description": "test cluster"
    }
    """
    And "worker_task_id1" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/alert-group" with data
    """
    {
		"monitoringFolderId": "test1",
		"alerts": [
			{
				"notificationChannels": [ "testch11", "testch22" ],
				"disabled": false,
				"templateID": "Postgresql master alive",
				"criticalThreshold": 22
			},
			{
				"notificationChannels": [ "testch11", "testch22" ],
				"disabled": false,
				"templateID": "Postgresql master walsize",
				"criticalThreshold": 33
			}
		]
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/alert-group/alert_group1" with data
    """
    {
		"alerts": [
			{
				"criticalThreshold": 22,
				"notificationChannels": [
					"testch111",
					"testch222"
				],
				"templateID": "Postgresql disk free bytes"
			},
			{
				"templateID": "Postgresql master alive",
				"notificationChannels": [
					"testch111",
					"testch222"
				],
				"criticalThreshold": 22
			}
		]
    }
    """
	Then we get response with status 200 and body contains
	"""
	{
		"createdBy": "user",
		"description": "Modify Alert Group for PostgreSQL cluster",
		"done": false,
		"id": "worker_task_id3",
		"metadata": {
			"@type": "yandex.cloud.mdb.postgresql.v1.ModifyAlertsGroupClusterMetadata",
			"clusterId": "cid1"
		}
	}
	"""
    And "worker_task_id3" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1/alert-groups"
	Then we get response with status 200 and body contains
	"""
	{
		"alertGroups": [
			{
				"alertGroupId":         "alert_group1",
				"monitoringFolderId":   "test1",
				"alerts": [
					{
						"notificationChannels": [ "testch111", "testch222" ],
						"disabled": false,
						"criticalThreshold": 22,
						"warningThreshold": null,
						"templateID": "Postgresql disk free bytes"
					},
					{
						"templateID": "Postgresql master alive",
						"warningThreshold": null,
						"criticalThreshold": 22,
						"notificationChannels": [ "testch111", "testch222" ],
						"disabled": false
					}
				]
			}
		]
	}
	"""
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/alert-group/alert_group1" with data
    """
    {
		"alerts": [
			{
				"notificationChannels": [ "testch111", "testch222" ],
				"disabled": false,
				"warningThreshold": 17,
				"criticalThreshold": 2277,
				"templateID": "Postgresql disk free bytes"
			},
			{
				"templateID": "Postgresql master alive",
				"warningThreshold": 1,
				"criticalThreshold": 9,
				"notificationChannels": [ "testch111", "testch222" ],
				"disabled": false
			}
		]
    }
    """
	Then we get response with status 200
    When we GET "/mdb/postgresql/1.0/clusters/cid1/alert-groups"
	Then we get response with status 200 and body contains
	"""
	{
		"alertGroups": [
			{
				"alertGroupId":         "alert_group1",
				"monitoringFolderId":   "test1",
				"alerts": [
					{
						"notificationChannels": [ "testch111", "testch222" ],
						"disabled": false,
						"criticalThreshold": 2277,
						"warningThreshold": 17,
						"templateID": "Postgresql disk free bytes"
					},
					{
						"templateID": "Postgresql master alive",
						"warningThreshold": 1,
						"criticalThreshold": 9,
						"notificationChannels": [ "testch111", "testch222" ],
						"disabled": false
					}
				]
			}
		]
	}
	"""
  Scenario: Alert Group monitoring folder update not works
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password"
       }],
       "hostSpecs": [{
           "zoneId": "myt"
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
       "description": "test cluster"
    }
    """
    And "worker_task_id1" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/alert-group" with data
    """
    {
		"monitoringFolderId": "test1",
		"alerts": [
			{
				"notificationChannels": [ "testch11", "testch22" ],
				"disabled": false,
				"templateID": "Postgresql master alive",
				"criticalThreshold": 22
			},
			{
				"notificationChannels": [ "testch11", "testch22" ],
				"disabled": false,
				"templateID": "Postgresql master walsize",
				"criticalThreshold": 33
			}
		]
    }
    """
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/alert-group/alert_group1" with data
    """
    {
		"monitoringFolderId": "test12",
		"alerts": [
			{
				"criticalThreshold": 22,
				"notificationChannels": [
					"testch111",
					"testch222"
				],
				"templateID": "Postgresql disk free bytes"
			},
			{
				"templateID": "Postgresql master alive",
				"notificationChannels": [
					"testch111",
					"testch222"
				],
				"criticalThreshold": 22
			}
		]
    }
    """
	Then we get response with status 422 and body contains
	"""
	{
		"code": 3,
		"message": "modifying monitoring folder id is not supported yet"
	}
	"""

  @alert_group_disable
  Scenario: Alert Group disable works
	When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
	"""
	{
		"name": "test2",
		"environment": "PRESTABLE",
		"configSpec": {
			"version": "14",
			"postgresqlConfig_14": {},
			"resources": {
				"resourcePresetId": "s1.porto.1",
				"diskTypeId": "local-ssd",
				"diskSize": 10737418240
			}
		},
		"databaseSpecs": [{
			"name": "testdb",
			"owner": "test"
		}],
		"userSpecs": [{
			"name": "test",
			"password": "test_password"
		}],
		"hostSpecs": [{
			"zoneId": "myt",
			"priority": 9,
			"configSpec": {
				"postgresqlConfig_14": {
					"workMem": 65536
				}
			}
		}, {
			"zoneId": "iva"
		}, {
			"zoneId": "sas"
		}],
		"description": "test cluster",
		"networkId": "IN-PORTO-NO-NETWORK-API",
		"defaultAlertGroup": {
			"monitoringFolderId": "folder2",
			"alerts": [
				{
					"notificationChannels": [ "ch1" ],
					"disabled": false,
					"warningThreshold": 17,
					"criticalThreshold": 2277,
					"templateID": "Postgresql disk free bytes"
				},
				{
					"templateID": "Postgresql master alive",
					"warningThreshold": 1,
					"criticalThreshold": 9,
					"notificationChannels": [ "ch1" ],
					"disabled": false
				}
			]
		}
    }
    """
    And "worker_task_id1" acquired and finished by worker
	When we PATCH "/mdb/postgresql/1.0/clusters/cid1/alert-group/alert_group1" with data
	"""
	{
		"alerts": [
			{
				"disabled": true,
				"templateID": "Postgresql disk free bytes"
			}
		]
	}
	"""
	Then we get response with status 200
	When we GET "/mdb/postgresql/1.0/clusters/cid1/alert-groups"
	Then we get response with status 200 and body contains
	"""
	{
		"alertGroups": [
			{
				"alertGroupId":         "alert_group1",
				"monitoringFolderId":   "folder2",
				"alerts": [
					{
						"notificationChannels": [ "ch1" ],
						"disabled": true,
						"warningThreshold": 17,
						"criticalThreshold": 2277,
						"templateID": "Postgresql disk free bytes"
					}
				]
			}
		]
	}
	"""

  @alerts_on_create
  Scenario: Create cluster with alert group works
    Given "create" data
    """
    {
       "name": "test2",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password"
       }],
       "hostSpecs": [{
           "zoneId": "myt",
           "priority": 9,
           "configSpec": {
                "postgresqlConfig_14": {
                    "workMem": 65536
                }
           }
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
		"description": "test cluster",
		"networkId": "IN-PORTO-NO-NETWORK-API",
		"defaultAlertGroup": {
				"monitoringFolderId": "folder2",
				"alerts": [
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"warningThreshold": 17,
						"criticalThreshold": 2277,
						"templateID": "Postgresql disk free bytes"
					},
					{
						"templateID": "Postgresql master alive",
						"warningThreshold": 1,
						"criticalThreshold": 9,
						"notificationChannels": [ "ch1" ],
						"disabled": false
					}
			]
		}
    }
    """
	When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1/alert-groups"
	Then we get response with status 200 and body contains
	"""
	{
		"alertGroups": [
			{
				"alertGroupId":       "alert_group1",
				"monitoringFolderId":   "folder2",
				"alerts": [
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"warningThreshold": 17,
						"criticalThreshold": 2277,
						"templateID": "Postgresql disk free bytes"
					},
					{
						"templateID": "Postgresql master alive",
						"warningThreshold": 1,
						"criticalThreshold": 9,
						"notificationChannels": [ "ch1" ],
						"disabled": false
					}
				]
			}
		]
	}
	"""
  @managed_alert_group
  Scenario: Delete managed alert-group fails
	Given "create" data
	"""
	{
	   "name": "test2",
	   "environment": "PRESTABLE",
	   "configSpec": {
		   "version": "14",
		   "postgresqlConfig_14": {},
		   "resources": {
			   "resourcePresetId": "s1.porto.1",
			   "diskTypeId": "local-ssd",
			   "diskSize": 10737418240
		   }
	   },
	   "databaseSpecs": [{
		   "name": "testdb",
		   "owner": "test"
	   }],
	   "userSpecs": [{
		   "name": "test",
		   "password": "test_password"
	   }],
	   "hostSpecs": [{
		   "zoneId": "myt",
		   "priority": 9,
		   "configSpec": {
				"postgresqlConfig_14": {
					"workMem": 65536
				}
		   }
		}, {
		   "zoneId": "iva"
		}, {
		   "zoneId": "sas"
		}],
		"description": "test cluster",
		"networkId": "IN-PORTO-NO-NETWORK-API",
		"defaultAlertGroup": {
				"monitoringFolderId": "folder2",
				"alerts": [
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"templateID": "Postgresql disk free bytes",
						"warningThreshold": 1,
						"criticalThreshold": 1
					},
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"criticalThreshold": 1,
						"templateID": "Postgresql master alive"
					},
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"warningThreshold": 1,
						"criticalThreshold": 1,
						"templateID": "Postgresql master walsize"
					}
				]
		}
    }
    """
	When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200
    When "worker_task_id1" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1/alert-groups"
	Then we get response with status 200 and body contains
	"""
	{
		"alertGroups": [
			{
				"alertGroupId":         "alert_group1",
				"monitoringFolderId":   "folder2",
				"alerts": [
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"templateID": "Postgresql disk free bytes",
						"warningThreshold": 1,
						"criticalThreshold": 1
					},
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"warningThreshold": null,
						"criticalThreshold": 1,
						"templateID": "Postgresql master alive"
					},
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"warningThreshold": 1,
						"criticalThreshold": 1,
						"templateID": "Postgresql master walsize"
					}
				]
			}
		]
	}
	"""
    When we DELETE "/mdb/postgresql/1.0/clusters/cid1/alert-group/alert_group1"
	Then we get response with status 422 and body contains
    """
    {
		"code" : 9,
		"message": "Deletion of managed alert group 'alert_group1' is prohibited"
    }
	"""
  @alert_template
  Scenario: List alert template works
    When we GET "/mdb/postgresql/1.0/alerts-template"
	Then we get response with status 200 and body contains
	"""
	{
		"alerts": [
			{
				"templateID": "Postgresql disk free bytes",
				"mandatory": false,
				"warningThreshold": 1,
				"criticalThreshold": 1,
				"name": "PostgreSQL master disk free space",
				"description": "PG master disk free space [byte]"
			},
			{
				"mandatory": false,
				"templateID": "Postgresql master alive",
				"warningThreshold": null,
				"criticalThreshold": 1,
				"name": "PostgreSQL master alive",
				"description": "PG master alive or not [bool]"
			},
			{
				"mandatory": false,
				"templateID": "Postgresql master walsize",
				"warningThreshold": 1,
				"criticalThreshold": 1,
				"name": "PostgreSQL master wal size",
				"description": "PG master wal size [byte]"
			}
		]
	}
	"""
  @alerts_restore
  Scenario: Restoring from backup with managed alert group works
    Given "create" data
    """
    {
       "name": "test2",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password"
       }],
       "hostSpecs": [{
           "zoneId": "myt",
           "priority": 9,
           "configSpec": {
                "postgresqlConfig_14": {
                    "workMem": 65536
                }
           }
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
		"description": "test cluster",
		"networkId": "IN-PORTO-NO-NETWORK-API",
		"defaultAlertGroup": {
				"monitoringFolderId": "folder2",
				"alerts": [
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"templateID": "Postgresql disk free bytes",
						"warningThreshold": 1,
						"criticalThreshold": 1
					},
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"criticalThreshold": 1,
						"templateID": "Postgresql master alive"
					},
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"warningThreshold": 1,
						"criticalThreshold": 1,
						"templateID": "Postgresql master walsize"
					}
				]
		}
    }
    """
	When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker
    When I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb),
		('auto02', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    When we GET "/mdb/postgresql/1.0/clusters/cid1/alert-groups"
	Then we get response with status 200 and body contains
	"""
	{
		"alertGroups": [
			{
				"alertGroupId":         "alert_group1",
				"monitoringFolderId":   "folder2",
				"alerts": [
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"templateID": "Postgresql disk free bytes",
						"warningThreshold": 1,
						"criticalThreshold": 1
					},
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"warningThreshold": null,
						"criticalThreshold": 1,
						"templateID": "Postgresql master alive"
					},
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"warningThreshold": 1,
						"criticalThreshold": 1,
						"templateID": "Postgresql master walsize"
					}
				]
			}
		]
	}
	"""
    When we POST "/mdb/postgresql/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:auto01",
        "time": "1970-02-01T00:00:02+00:00"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new PostgreSQL cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto01",
            "clusterId": "cid2"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.RestoreCluster" event with
    """
    {
        "details": {
            "backup_id": "cid1:auto01",
            "cluster_id": "cid2"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid2/alert-groups"
	Then we get response with status 200 and body contains
	"""
	{
		"alertGroups": [
			{
				"alertGroupId":         "alert_group2",
				"monitoringFolderId":   "folder2",
				"alerts": [
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"templateID": "Postgresql disk free bytes",
						"warningThreshold": 1,
						"criticalThreshold": 1
					},
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"warningThreshold": null,
						"criticalThreshold": 1,
						"templateID": "Postgresql master alive"
					},
					{
						"notificationChannels": [ "ch1" ],
						"disabled": false,
						"warningThreshold": 1,
						"criticalThreshold": 1,
						"templateID": "Postgresql master walsize"
					}
				]
			}
		]
	}
	"""
    When we DELETE "/mdb/postgresql/1.0/clusters/cid2/alert-group/alert_group1"
	Then we get response with status 422 and body contains
    """
    {
		"code" : 9,
		"message": "Deletion of managed alert group 'alert_group1' is prohibited"
    }
	"""
  # this test should be changed after we enforce contraint that every cluster should have at least one managed ag
  Scenario: Restoring from backup without managed alert group not fails
    Given "create" data
    """
    {
       "name": "test2",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password"
       }],
       "hostSpecs": [{
           "zoneId": "myt",
           "priority": 9,
           "configSpec": {
                "postgresqlConfig_14": {
                    "workMem": 65536
                }
           }
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }]
    }
    """
	When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker
    When I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb),
		('auto02', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    When we GET "/mdb/postgresql/1.0/clusters/cid1/alert-groups"
	Then we get response with status 200 and body contains
	"""
	{
		"alertGroups": [
		]
	}
	"""
    When we POST "/mdb/postgresql/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:auto01",
        "time": "1970-02-01T00:00:02+00:00"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new PostgreSQL cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto01",
            "clusterId": "cid2"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.RestoreCluster" event with
    """
    {
        "details": {
            "backup_id": "cid1:auto01",
            "cluster_id": "cid2"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid2/alert-groups"
	Then we get response with status 200 and body contains
	"""
	{
		"alertGroups": [
		]
	}
	"""
