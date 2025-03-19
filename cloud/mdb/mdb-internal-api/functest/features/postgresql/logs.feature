Feature: PostgreSQL logs

  Background:
    Given default headers
    And "create" data
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
       "networkId": "IN-PORTO-NO-NETWORK-API"
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

  Scenario Outline: Request for logs with invalid parameters fails (gRPC)
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.postgresql.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        <filter>
        <page token>
        "service_type": "<service type>"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "<message>"
    Examples:
      | service type | filter                          | page token          | message                                                                                                           |
      | POSTGRESQL   | "column_filter": ["nocolumn"],  |                     | invalid column "nocolumn", valid columns: ["application_name", "command_tag", "connection_from", "context", "database_name", "detail", "error_severity", "hint", "hostname", "internal_query", "internal_query_pos", "location", "message", "process_id", "query", "query_pos", "session_id", "session_line_num", "session_start_time", "sql_state_code", "transaction_id", "user_name", "virtual_transaction_id"] |
      | POOLER       | "column_filter": ["nocolumn"],  |                     | invalid column "nocolumn", valid columns: ["client_id", "context", "db", "hostname", "level", "pid", "server_id", "text", "user"] |
      | POSTGRESQL   |                                 | "page_token": "A",  | invalid page token                                                                                                |
      | POOLER       |                                 | "page_token": "A",  | invalid page token                                                                                                |
