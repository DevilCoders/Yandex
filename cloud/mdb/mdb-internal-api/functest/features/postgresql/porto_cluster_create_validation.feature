Feature: Create Porto PostgreSQL Cluster Validation
  Background:
    Given default headers

  Scenario: Create cluster with insufficient quota fails
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
           "resources": {
               "resourcePresetId": "s1.porto.5",
               "diskTypeId": "local-ssd",
               "diskSize": 1073741824000
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Quota limits exceeded, not enough cpu: 24 cores, memory: 96 GiB, ssd_space: 2.34 TiB",
        "details": [
            {
                "@type": "type.private-api.yandex-cloud.ru/quota.QuotaFailure",
                "cloud_id": "cloud1",
                "violations": [
                    {
                        "metric": {
                            "limit": 24,
                            "name": "mdb.cpu.count",
                            "usage": 0
                        },
                        "required": 48
                    },
                    {
                        "metric": {
                            "limit": 103079215104,
                            "name": "mdb.memory.size",
                            "usage": 0
                        },
                        "required": 206158430208
                    },
                    {
                        "metric": {
                            "limit": 644245094400,
                            "name": "mdb.ssd.size",
                            "usage": 0
                        },
                        "required": 3221225472000
                    }
                ]
            }
        ]
    }
    """

  Scenario: Create cluster with nonexistent flavor fails
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
           "resources": {
               "resourcePresetId": "invalid",
               "diskTypeId": "local-ssd",
               "diskSize": 1073741824000
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
       }],
       "description": "test cluster"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "resourcePreset 'invalid' does not exist"
    }
    """

  Scenario: Create cluster with read-only token fails
    Given default headers with "ro-token" token
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
               "diskSize": 1073741824000
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
    Then we get response with status 403 and body contains
    """
    {
        "code": 7,
        "message": "You do not have permission to access the requested object or object does not exist"
    }
    """

  Scenario: Create cluster with user without password fails
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
               "diskSize": 1073741824000
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test"
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nuserSpecs.0.password: Missing data for required field."
    }
    """

  Scenario: Create cluster with invalid database name fails
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
               "diskSize": 1073741824000
           }
       },
       "databaseSpecs": [{
           "name": "b@dn@me!",
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\ndatabaseSpecs.0.name: Database name 'b@dn@me!' does not conform to naming rules"
    }
    """

  Scenario: Create cluster with too large user conns fails
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
           "password": "test_password",
           "connLimit": 1000
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "max_connections (200) is less than sum of users connection limit (1015)"
    }
    """
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {
               "maxConnections": 100
           },
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
       },{
           "name": "test1",
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "max_connections (100) is less than sum of users connection limit (115)"
    }
    """

  Scenario: Create cluster with max_wal_size greater then DEFAULT_FRAC_DISK_SIZE_WAL_LIMIT of space limit fails
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {
               "maxWalSize": 6442450944,
               "minWalSize": 1048576000
           },
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
           "password": "test_password",
           "connLimit": 10
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "max_wal_size (6442450944) is greater than 0.5 of disk_size (10737418240)"
    }
    """
  Scenario: Create cluster with min_wal_size less then DEFAULT_MIN_WAL_SIZE_IN_MEGABYTE fails
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {
               "maxWalSize": 1073741824,
               "minWalSize": 66060288
           },
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
           "password": "test_password",
           "connLimit": 10
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "min_wal_size (66060288) is less than minimum allowed 64 MEGABYTES"
    }
    """

  Scenario: Create cluster with max_wal_size less then min_wal_size fails
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {
               "maxWalSize": 838860800,
               "minWalSize": 1048576000
           },
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
           "password": "test_password",
           "connLimit": 10
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "max_wal_size (838860800) is less than min_wal_size (1048576000)"
    }
    """
  Scenario: Create cluster with wal_level
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {
               "walLevel": "WAL_LEVEL_REPLICA"
           },
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
           "password": "test_password",
           "connLimit": 10
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "walLevel is deprecated and no longer supported"
    }
    """

  Scenario: Create cluster with missing db for user fails
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
           "password": "test_password",
           "permissions": [{
               "databaseName": "testdb2"
           }]
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The specified permission for user 'test' is invalid. The database 'testdb2' does not exist."
    }
    """

  Scenario: Create cluster with missing disk_size fails
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
               "disk_size": 0
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.resources.diskSize: Missing data for required field."
    }
    """

  Scenario: Create cluster with host in unknown zone fails
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
               "diskSize": 1073741824000
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
           "zoneId": "nodc"
       }],
       "description": "test cluster"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nhostSpecs.0.zoneId: Invalid value, valid value is one of ['iva', 'man', 'myt', 'sas', 'vla']"
    }
    """

  Scenario: Create cluster with incorrect version fails
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "1.1",
           "resources": {
               "resourcePresetId": "s1.compute.1",
               "diskTypeId": "network-ssd",
               "diskSize": 107374182400
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
           "zoneId": "man"
       }],
       "description": "test cluster",
       "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Version '1.1' is not available, allowed versions are: 10, 11, 12, 13, 14"
    }
    """

  @decommission @geo
  Scenario: Create cluster with node in decommissioning geo
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
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
           "zoneId": "ugr"
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
       "description": "test cluster",
       "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "No new resources could be created in zone 'ugr'"
    }
    """

  Scenario: Create cluster more then 17 ha hosts fails
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
               "diskSize": 107374182400
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
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }, {
           "zoneId": "man"
       }],
       "description": "test cluster",
       "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "PostgreSQL cluster allows at most 17 HA hosts"
    }
    """
