@sqlserver
@grpc_api
Feature: Create, delete and modify SQL Server users
  Background:
    Given default headers
    When we add default feature flag "MDB_SQLSERVER_CLUSTER"
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
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
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateClusterMetadata",
        "cluster_id": "cid1"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id1" acquired and finished by worker

  Scenario: Delete not existing user fails
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "UserNotFound"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "user "UserNotFound" not found"

  Scenario: Delete system user fails
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "sa"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "invalid user name "sa""

  Scenario: User creation and delete works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "testtwo",
        "password": "TestPassword",
        "permissions": [
          {
            "database_name": "testdb",
            "roles": [
                "DB_DDLADMIN",
                "DB_DATAREADER",
                "DB_DATAWRITER",
                "DB_DENYDATAWRITER"
            ]
          }
        ],
        "server_roles": ["MDB_MONITOR"]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create user in Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateUserMetadata",
        "cluster_id": "cid1",
        "user_name": "testtwo"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id2" acquired and finished by worker

    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "page_size": 10
    }
    """
    Then we get gRPC response with body
    """
    {
      "users": [
        {
          "name": "test",
          "cluster_id": "cid1",
          "permissions": [],
          "server_roles": []
        },
        {
          "name": "testtwo",
          "cluster_id": "cid1",
          "permissions": [
            {
              "database_name": "testdb",
              "roles": [
                    "DB_DDLADMIN",
                    "DB_DATAREADER",
                    "DB_DATAWRITER",
                    "DB_DENYDATAWRITER"
              ]
            }
          ],
          "server_roles": [
              "MDB_MONITOR"
          ]
        }
      ]
    }
    """

    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test",
      "permissions": [{
              "database_name": "testdb",
              "roles": [
                    "DB_DENYDATAWRITER"
              ]
          }
      ],
      "update_mask": {
        "paths": [
          "permissions"
        ]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Modify user in Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.UpdateUserMetadata",
        "cluster_id": "cid1",
        "user_name": "test"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test"
    }
    """
    Then we get gRPC response with body
    """
    {
      "name": "test",
      "cluster_id": "cid1",
      "permissions": [{
          "database_name": "testdb",
          "roles": [
                "DB_DENYDATAWRITER"
          ]
        }
      ],
      "server_roles": []
    }
    """

    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Delete user from Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id4",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.DeleteUserMetadata",
        "cluster_id": "cid1",
        "user_name": "test"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id4" acquired and finished by worker
    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "page_size": 10
    }
    """
    Then we get gRPC response with body
    """
    {
      "users": [
        {
          "name": "testtwo",
          "cluster_id": "cid1",
          "permissions": [
            {
              "database_name": "testdb",
              "roles": [
                    "DB_DDLADMIN",
                    "DB_DATAREADER",
                    "DB_DATAWRITER",
                    "DB_DENYDATAWRITER"
              ]
            }
          ],
          "server_roles": ["MDB_MONITOR"]
        }
      ]
    }
    """

  Scenario: Empty user update with fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "no changes detected"

  Scenario: User update with not valid password fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test",
      "password": "1",
      "update_mask": {
        "paths": [
          "password"
        ]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "password is too short"

Scenario: User update with tab symbol in password fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test",
      "password": "P@ssw0rd123 \t jkljaawe4324123",
      "update_mask": {
        "paths": [
          "password"
        ]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "password has invalid symbols"

Scenario: User update with slash symbol in password fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test",
      "password": "P@ssw0rd123 \\ jkljaawe4324123",
      "update_mask": {
        "paths": [
          "password"
        ]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "password has invalid symbols"

Scenario: User update with \0 symbol in password fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test",
      "password": "P@ssw0rd123\\0jkljaawe4324123",
      "update_mask": {
        "paths": [
          "password"
        ]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "password has invalid symbols"

Scenario: User update with \' symbol in password fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test",
      "password": "P@ssw0rd123\\'jkljaawe4324123",
      "update_mask": {
        "paths": [
          "password"
        ]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "password has invalid symbols"

Scenario: User update with \\" symbol in password fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test",
      "password": "P@ssw0rd123\\\"jkljaawe4324123",
      "update_mask": {
        "paths": [
          "password"
        ]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "password has invalid symbols"

Scenario: User update with \\n symbol in password fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test",
      "password": "P@ssw0rd123jkljaawe4324123\\n",
      "update_mask": {
        "paths": [
          "password"
        ]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "password has invalid symbols"

Scenario: User update with \\r symbol in password fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test",
      "password": "P@ssw0rd123jkljaawe4324123\\r",
      "update_mask": {
        "paths": [
          "password"
        ]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "password has invalid symbols"

Scenario: User update with \\x1A symbol in password fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test",
      "password": "P@ssw0rd123jkljaawe4324123\\x1A",
      "update_mask": {
        "paths": [
          "password"
        ]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "password has invalid symbols"

Scenario: User update with \\x08 symbol in password fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test",
      "password": "P@ssw0rd123jkljaawe4324123\\x08",
      "update_mask": {
        "paths": [
          "password"
        ]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "password has invalid symbols"

  Scenario: User update password with invalid user fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "testIncorrect",
      "password": "verysecurePaswoworhf372323627!!!@@@@",
      "update_mask": {
        "paths": [
          "password"
        ]
      }
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "user "testIncorrect" not found"

  Scenario: User grant/revoke permission works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "testtwo",
        "password": "TestPassword",
        "permissions": [
          {
            "database_name": "testdb",
            "roles": [
                "DB_DDLADMIN",
                "DB_DATAREADER",
                "DB_DATAWRITER",
                "DB_DENYDATAWRITER"
            ]
          }
        ]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create user in Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateUserMetadata",
        "cluster_id": "cid1",
        "user_name": "testtwo"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id2" acquired and finished by worker

    When we "GrantPermission" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "testtwo",
      "permission": {
            "database_name": "testdb",
            "roles": [
                "DB_DENYDATAREADER"
            ]
       }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Grant user permissions in Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.UpdateUserMetadata",
        "cluster_id": "cid1",
        "user_name": "testtwo"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "testtwo"
    }
    """
    Then we get gRPC response with body
    """
    {
        "name": "testtwo",
        "cluster_id": "cid1",
        "permissions": [
            {
              "database_name": "testdb",
              "roles": [
                    "DB_DDLADMIN",
                    "DB_DATAREADER",
                    "DB_DATAWRITER",
                    "DB_DENYDATAWRITER",
                    "DB_DENYDATAREADER"
              ]
            }
          ],
        "server_roles": []
    }
    """
    When we "RevokePermission" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "testtwo",
      "permission": {
            "database_name": "testdb",
            "roles": [
                "DB_DATAREADER"
            ]
       }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Revoke user permissions in Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id4",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.UpdateUserMetadata",
        "cluster_id": "cid1",
        "user_name": "testtwo"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id4" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "testtwo"
    }
    """
    Then we get gRPC response with body
    """
    {
        "name": "testtwo",
        "cluster_id": "cid1",
        "permissions": [
            {
                "database_name": "testdb",
                "roles": [
                    "DB_DDLADMIN",
                    "DB_DATAWRITER",
                    "DB_DENYDATAWRITER",
                    "DB_DENYDATAREADER"
                ]
            }
          ],
        "server_roles": []
    }
    """

  Scenario: User grant with incorrect username fails
    When we "GrantPermission" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "testIncorrect",
      "permission": {
            "database_name": "testdb",
            "roles": [
                "DB_DENYDATAREADER"
            ]
       }
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "user "testIncorrect" not found"

  Scenario: User revoke with incorrect username fails
    When we "RevokePermission" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "testIncorrect",
      "permission": {
            "database_name": "testdb",
            "roles": [
                "DB_DENYDATAREADER"
            ]
       }
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "user "testIncorrect" not found"

  Scenario: User grant with incorrect database fails
    When we "GrantPermission" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test",
      "permission": {
            "database_name": "testdbIncorrect",
            "roles": [
                "DB_DENYDATAREADER"
            ]
       }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "permission refers not existing database: testdbIncorrect"


  Scenario: User revoke with incorrect database fails
    When we "RevokePermission" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test",
      "permission": {
            "database_name": "testdbIncorrect",
            "roles": [
                "DB_DENYDATAREADER"
            ]
       }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "permission refers not existing database: testdbIncorrect"

  Scenario: Creating cluster with server roles works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "testtwo",
        "password": "TestPassword",
        "permissions": [
          {
            "database_name": "testdb",
            "roles": [
                "DB_DDLADMIN",
                "DB_DATAREADER",
                "DB_DATAWRITER",
                "DB_DENYDATAWRITER"
            ]
          }
        ],
        "server_roles": ["MDB_MONITOR"]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create user in Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateUserMetadata",
        "cluster_id": "cid1",
        "user_name": "testtwo"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "page_size": 10
    }
    """
    Then we get gRPC response with body
    """
    {
      "users": [
        {
          "name": "test",
          "cluster_id": "cid1",
          "permissions": [],
          "server_roles": []
        },
        {
          "name": "testtwo",
          "cluster_id": "cid1",
          "permissions": [
            {
              "database_name": "testdb",
              "roles": [
                    "DB_DDLADMIN",
                    "DB_DATAREADER",
                    "DB_DATAWRITER",
                    "DB_DENYDATAWRITER"
              ]
            }
          ],
          "server_roles": [
              "MDB_MONITOR"
          ]
        }
      ]
    }
    """

  Scenario: Granting server roles works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "testthree",
        "password": "TestPassword",
        "permissions": [
          {
            "database_name": "testdb",
            "roles": [
                "DB_DDLADMIN",
                "DB_DATAREADER",
                "DB_DATAWRITER",
                "DB_DENYDATAWRITER"
            ]
          }
        ]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create user in Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateUserMetadata",
        "cluster_id": "cid1",
        "user_name": "testthree"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "page_size": 10
    }
    """
    Then we get gRPC response with body
    """
    {
      "users": [
        {
          "name": "test",
          "cluster_id": "cid1",
          "permissions": [],
          "server_roles": []
        },
        {
          "name": "testthree",
          "cluster_id": "cid1",
          "permissions": [
            {
              "database_name": "testdb",
              "roles": [
                    "DB_DDLADMIN",
                    "DB_DATAREADER",
                    "DB_DATAWRITER",
                    "DB_DENYDATAWRITER"
              ]
            }
          ],
          "server_roles": []
        }
      ]
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "testthree",
      "server_roles": ["MDB_MONITOR"],
      "update_mask": {
        "paths": [
          "server_roles"
        ]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Modify user in Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.UpdateUserMetadata",
        "cluster_id": "cid1",
        "user_name": "testthree"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "testthree"
    }
    """
    Then we get gRPC response with body
    """
    {
      "name": "testthree",
      "cluster_id": "cid1",
      "permissions": [{
          "database_name": "testdb",
          "roles": [
                "DB_DDLADMIN",
                "DB_DATAREADER",
                "DB_DATAWRITER",
                "DB_DENYDATAWRITER"
          ]
        }
      ],
      "server_roles": [
          "MDB_MONITOR"
      ]
    }
    """

# We need to test server roles revoke
