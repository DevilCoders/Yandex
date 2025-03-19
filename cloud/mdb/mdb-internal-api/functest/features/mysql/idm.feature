Feature: IDM users feature for MySQL Cluster
  Background:
    Given default headers
    And "create_with_idm" data
    """
    {
        "name": "test_porto",
        "environment": "PRODUCTION",
        "configSpec": {
            "mysqlConfig_5_7": {
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskTypeId": "local-ssd",
                "diskSize": 10737418240
            },
            "soxAudit": true
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "zoneId": "myt",
            "backupPriority": 5
        }, {
            "zoneId": "iva",
            "priority": 7
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API",
        "deletionProtection": false
    }
    """

Scenario: Cluster creation with IDM flag FALSE does not work in porto PRODUCTION
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRODUCTION",
        "configSpec": {
            "mysqlConfig_5_7": {
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskTypeId": "local-ssd",
                "diskSize": 10737418240
            },
            "soxAudit": false
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "zoneId": "myt",
            "backupPriority": 5
        }, {
            "zoneId": "iva",
            "priority": 7
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API",
        "deletionProtection": false
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "IDM flag is required. To use cluster without flag you should get an approval in MDBSUPPORT ticket."
    }
    """

Scenario: Cluster creation without IDM flag works in porto PRODUCTION because of default value
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRODUCTION",
        "configSpec": {
            "mysqlConfig_5_7": {
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskTypeId": "local-ssd",
                "diskSize": 10737418240
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "zoneId": "myt",
            "backupPriority": 5
        }, {
            "zoneId": "iva",
            "priority": 7
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API",
        "deletionProtection": false
    }
    """
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker

Scenario: Cluster creation without IDM flag works in compute PRODUCTION
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRODUCTION",
        "configSpec": {
            "mysqlConfig_5_7": {
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "network-ssd",
                "diskSize": 10737418240
            },
            "soxAudit": false
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "vla"
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "network1",
        "securityGroupIds": ["sg_id1", "sg_id2"],
        "deletionProtection": false
    }
    """
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker


Scenario: Cluster creation with IDM flag works in porto PRODUCTION
    Given default headers
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with "create_with_idm" data
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "environment": "PRODUCTION",
        "folderId": "folder1",
        "id": "cid1",
        "name": "test_porto"
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/users"
    Then we get response with status 200 and body contains
    """
    {
        "users": [
            {
                "clusterId": "cid1",
                "name": "mdb_reader",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["SELECT"]
                    }
                ]
            },
            {
                "clusterId": "cid1",
                "name": "mdb_writer",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["ALL_PRIVILEGES"]
                    }
                ]
            },
            {
                "clusterId": "cid1",
                "name": "test",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["ALL_PRIVILEGES"]
                    }
                ]
            }
        ]
    }
    """


Scenario: Cluster IDM flag disable disallowed in porto PRODUCTION
    Given default headers
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with "create_with_idm" data
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "soxAudit": false
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "IDM flag is required. To use cluster without flag you should get an approval in MDBSUPPORT ticket."
    }
    """

Scenario: IDM system user edit allowed
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with "create_with_idm" data
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we PATCH "/mdb/mysql/1.0/clusters/cid1/users/mdb_reader" with data
    """
    {
        "permissions": [
            {
                "databaseName": "testdb",
                "roles": ["SELECT", "SHOW_VIEW"]
            }
        ],
        "connectionLimits": {
            "maxQuestionsPerHour": 2,
            "maxUpdatesPerHour": 10,
            "maxUserConnections": 20
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "mdb_reader"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/mysql/1.0/clusters/cid1/users/mdb_reader"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "mdb_reader",
        "permissions": [
            {
                "databaseName": "testdb",
                "roles": ["SELECT", "SHOW_VIEW"]
            }
        ],
        "connectionLimits": {
            "maxQuestionsPerHour": 2,
            "maxUpdatesPerHour": 10,
            "maxUserConnections": 20
        }
    }
    """


Scenario: After enabling IDM in porto PRESTABLE IDM system users are created
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test_porto",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_5_7": {
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskTypeId": "local-ssd",
                "diskSize": 10737418240
            },
            "soxAudit": false
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "zoneId": "myt",
            "backupPriority": 5
        }, {
            "zoneId": "iva",
            "priority": 7
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API",
        "deletionProtection": false
    }
    """
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "id": "cid1",
        "name": "test_porto"
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/users"
    Then we get response with status 200 and body contains
    """
    {
        "users": [
            {
                "clusterId": "cid1",
                "name": "test",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["ALL_PRIVILEGES"]
                    }
                ]
            }
        ]
    }
    """
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "soxAudit": true
        }
    }
    """
    Then we get response with status 200
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/mysql/1.0/clusters/cid1/users"
    Then we get response with status 200 and body contains
    """
    {
        "users": [
            {
                "clusterId": "cid1",
                "name": "mdb_reader",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["SELECT"]
                    }
                ]
            },
            {
                "clusterId": "cid1",
                "name": "mdb_writer",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["ALL_PRIVILEGES"]
                    }
                ]
            },
            {
                "clusterId": "cid1",
                "name": "test",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["ALL_PRIVILEGES"]
                    }
                ]
            }
        ]
    }
    """


Scenario Outline: IDM system user password and plugin edit is forbidden
    Given default headers
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with "create_with_idm" data
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we PATCH "/mdb/mysql/1.0/clusters/cid1/users/<user_name>" with data
    """
    {
        "<field>": <value>
    }
    """
    Then we get response with status 403
    """
    {
        "code": 7,
        "message": "<message>"
    }
    """
    Examples:
    | user_name  | field                  | value             | message                                                      |
    | mdb_reader | password               | "12345678"        | IDM system user 'mdb_reader' 'password' editing is forbidden |
    | mdb_writer | password               | "12345678"        | IDM system user 'mdb_writer' 'password' editing is forbidden |
    | mdb_reader | authenticationPlugin   | "SHA256_PASSWORD" | IDM system user 'mdb_reader' 'plugin' editing is forbidden   |
    | mdb_writer | authenticationPlugin   | "SHA256_PASSWORD" | IDM system user 'mdb_writer' 'plugin' editing is forbidden   |


Scenario: IDM user edit is forbidden
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with "create_with_idm" data
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we run query
    """
    select code.easy_update_pillar('cid1', 'data:mysql:users:idm_user', '{"origin": "idm", "proxy_user": "mdb_reader", "password": "12345678"}'::jsonb);
    """
    And we GET "/mdb/mysql/1.0/clusters/cid1/users/idm_user"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "idm_user"
    }
    """
    When we PATCH "/mdb/mysql/1.0/clusters/cid1/users/idm_user" with data
    """
    {
        "permissions": [
            {
                "databaseName": "testdb",
                "roles": ["SELECT", "SHOW_VIEW"]
            }
        ],
        "connectionLimits": {
            "maxQuestionsPerHour": 2,
            "maxUpdatesPerHour": 10,
            "maxUserConnections": 20
        }
    }
    """
    Then we get response with status 403 and body contains
    """
    {
        "code": 7,
        "message": "IDM user 'idm_user' editing is forbidden"
    }
    """

Scenario: when adding DB default privileges are given to IDM system users
    Given default headers
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with "create_with_idm" data
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we POST "/mdb/mysql/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "testdb2"
        }
    }
    """
    Then we get response with status 200
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/mysql/1.0/clusters/cid1/users"
    Then we get response with status 200 and body contains
    """
    {
        "users": [
            {
                "clusterId": "cid1",
                "name": "mdb_reader",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["SELECT"]
                    },
                    {
                        "databaseName": "testdb2",
                        "roles": ["SELECT"]
                    }
                ]
            },
            {
                "clusterId": "cid1",
                "name": "mdb_writer",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["ALL_PRIVILEGES"]
                    },
                    {
                        "databaseName": "testdb2",
                        "roles": ["ALL_PRIVILEGES"]
                    }
                ]
            },
            {
                "clusterId": "cid1",
                "name": "test",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["ALL_PRIVILEGES"]
                    }
                ]
            }
        ]
    }
    """
    When we DELETE "/mdb/mysql/1.0/clusters/cid1/databases/testdb2"
    Then we get response with status 200
    And "worker_task_id3" acquired and finished by worker
    When we GET "/mdb/mysql/1.0/clusters/cid1/users"
    Then we get response with status 200 and body contains
    """
    {
        "users": [
            {
                "clusterId": "cid1",
                "name": "mdb_reader",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["SELECT"]
                    }
                ]
            },
            {
                "clusterId": "cid1",
                "name": "mdb_writer",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["ALL_PRIVILEGES"]
                    }
                ]
            },
            {
                "clusterId": "cid1",
                "name": "test",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["ALL_PRIVILEGES"]
                    }
                ]
            }
        ]
    }
    """
