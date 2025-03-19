@greenplum
@grpc_api
Feature: Greenplum logs
    Background:
        Given default headers
        When we add default feature flag "MDB_GREENPLUM_CLUSTER"
        And we "Create" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "folder_id": "folder1",
            "environment": "PRESTABLE",
            "name": "testName",
            "description": "test cluster description",
            "labels": {
                "foo": "bar"
            },
            "network_id": "network1",
            "config": {
                "zone_id": "myt",
                "subnet_id": "",
                "assign_public_ip": false
            },
            "master_config": {
                "resources": {
                    "resourcePresetId": "s1.compute.2",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "segment_config": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "master_host_count": 2,
            "segment_in_host": 1,
            "segment_host_count": 4,
            "user_name": "usr1",
            "user_password": "Pa$$w0rd"
        }
        """
        Then we get gRPC response with body
        """
        {
            "created_by": "user",
            "description": "Create Greenplum cluster",
            "done": false,
            "id": "worker_task_id1",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.CreateClusterMetadata",
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
    Scenario Outline: Request for greenplum pooler logs
        Given named logsdb response
        """
        [
            [
                "log_seconds",
                "log_milliseconds",
                "message",
                "ms",
                "hostname",
                "cluster",
                "origin",
                "text",
                "client_id",
                "server_id",
                "context",
                "db",
                "user",
                "pid",
                "level",
                "insert_time",
                "log_format"
            ],
            [
                0,
                0,
                "message 1",
                12,
                "my_super_host",
                "mdbtestcluster",
                "origin my super origin",
                "text message",
                "client_id",
                "server_id",
                "context",
                "db",
                "user",
                "pid12345",
                "level info",
                "1970-01-01T00:00:00Z",
                "log_format"
            ]
        ]
        """
        When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1",
            <filter>
            <page_token>
            "service_type": "GREENPLUM_POOLER"

        }
        """
        Then we get gRPC response with body
        """
        {
            "logs": [
                {
                    "message": {
                        <message>
                    },
                    "timestamp": "1970-01-01T00:00:00Z"
                }
            ]
        }
        """
        Examples:
            | filter                       | page_token         | message                                                                                                                                                                                                                                                                                                                                          |
            |                              |                    | "hostname": "my_super_host", "text": "text message", "client_id": "client_id", "server_id": "server_id", "context": "context", "db": "db", "user": "user", "pid": "pid12345", "level": "level info"                                                                                                                                              |
            | "column_filter": "hostname", | "page_token": "0", | "hostname": "my_super_host"                                                                                                                                                                                                                                                                                                                      |
            | "column_filter": "level",    | "page_token": "0", | "level": "level info"                                                                                                                                                                                                                                                                                                                            |
    Scenario Outline: Request for greenplum pooler logs with invalid parameters fails
        Given named logsdb response
        """
        [
            [
                "log_seconds",
                "log_milliseconds",
                "message",
                "ms",
                "hostname",
                "cluster",
                "origin",
                "text",
                "client_id",
                "server_id",
                "context",
                "db",
                "user",
                "pid",
                "level",
                "insert_time",
                "log_format"
            ],
            [
                0,
                0,
                "message 1",
                12,
                "my_super_host",
                "mdbtestcluster",
                "origin my super origin",
                "text message",
                "client_id",
                "server_id",
                "context",
                "db",
                "user",
                "pid12345",
                "level info",
                "1970-01-01T00:00:00Z",
                "log_format"
            ]
        ]
        """
        When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1",
            <filter>
            <page_token>
            "service_type": "GREENPLUM_POOLER"

        }
        """
        Then we get gRPC response error with code <error_code> and message "<message>"
        Examples:
            | filter                        | page_token         | error_code       | message                                                                                                                            |
            | "column_filter": "no column", | "page_token": "0", | INVALID_ARGUMENT | invalid column "no column", valid columns: ["client_id", "context", "db", "hostname", "level", "pid", "server_id", "text", "user"] |
    Scenario Outline: Request for greenplum logs
        Given named logsdb response
        """
        [
            [
                "log_seconds",
                "log_milliseconds",
                "cluster",
                "database_name",
                "debug_query_string",
                "distr_tranx_id",
                "error_cursor_pos",
                "event_context",
                "event_detail",
                "event_hint",
                "event_message",
                "event_severity",
                "event_time",
                "file_line",
                "file_name",
                "func_name",
                "gp_command_count",
                "gp_host_type",
                "gp_preferred_role",
                "gp_segment",
                "gp_session_id",
                "hostname",
                "insert_time",
                "internal_query",
                "internal_query_pos",
                "local_tranx_id",
                "log_format",
                "ms",
                "origin",
                "process_id",
                "remote_host",
                "remote_port",
                "session_start_time",
                "slice_id",
                "sql_state_code",
                "stack_trace",
                "sub_tranx_id",
                "thread_id",
                "transaction_id",
                "user_name"
            ],
            [
                0,
                0,
                "cid1",
                 "test_db",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "hello world",
                 "",
                 "",
                 "",
                 "",
                 "hello.c",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "123",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 ""
            ]
        ]
        """
        When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1",
            <filter>
            <page_token>
            "service_type": "GREENPLUM"

        }
        """
        Then we get gRPC response with body
        """
        {
            "logs": [
                {
                    "message": {
                        <message>
                    },
                    "timestamp": "1970-01-01T00:00:00Z"
                }
            ]
        }
        """
        Examples:
            | filter                       | page_token         | message                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
            |                              |                    | "cluster": "cid1", "database_name": "test_db", "debug_query_string": "", "distr_tranx_id": "", "error_cursor_pos": "", "event_context": "", "event_detail": "", "event_hint": "", "event_message": "hello world", "event_severity": "", "event_time": "", "file_line": "", "file_name": "", "func_name": "hello.c", "gp_command_count": "", "gp_host_type": "", "gp_preferred_role": "", "gp_segment": "", "gp_session_id": "", "hostname": "", "internal_query": "", "internal_query_pos": "", "local_tranx_id": "", "process_id": "123", "remote_host": "", "remote_port": "", "session_start_time": "", "slice_id": "", "sql_state_code": "", "stack_trace": "", "sub_tranx_id": "", "thread_id": "", "transaction_id": "", "user_name": "" |
    Scenario Outline: Request for greenplum logs with invalid parameters fails
        Given named logsdb response
        """
        [
            [
                "log_seconds",
                "log_milliseconds",
                "cluster",
                "database_name",
                "debug_query_string",
                "distr_tranx_id",
                "error_cursor_pos",
                "event_context",
                "event_detail",
                "event_hint",
                "event_message",
                "event_severity",
                "event_time",
                "file_line",
                "file_name",
                "func_name",
                "gp_command_count",
                "gp_host_type",
                "gp_preferred_role",
                "gp_segment",
                "gp_session_id",
                "hostname",
                "insert_time",
                "internal_query",
                "internal_query_pos",
                "local_tranx_id",
                "log_format",
                "ms",
                "origin",
                "process_id",
                "remote_host",
                "remote_port",
                "session_start_time",
                "slice_id",
                "sql_state_code",
                "stack_trace",
                "sub_tranx_id",
                "thread_id",
                "transaction_id",
                "user_name"
            ],
            [
                0,
                0,
                "cid1",
                 "test_db",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "hello world",
                 "my_event_severity",
                 "",
                 "",
                 "",
                 "hello.c",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "my_super_host",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "123",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 ""
            ]
        ]
        """
        When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1",
            <filter>
            <page_token>
            "service_type": "GREENPLUM"
        }
        """
        Then we get gRPC response error with code <error_code> and message "<message>"
        Examples:
            | filter                        | page_token         | error_code       | message                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
            | "column_filter": "no column", | "page_token": "0", | INVALID_ARGUMENT | invalid column "no column", valid columns: ["cluster", "database_name", "debug_query_string", "distr_tranx_id", "error_cursor_pos", "event_context", "event_detail", "event_hint", "event_message", "event_severity", "event_time", "file_line", "file_name", "func_name", "gp_command_count", "gp_host_type", "gp_preferred_role", "gp_segment", "gp_session_id", "hostname", "internal_query", "internal_query_pos", "local_tranx_id", "process_id", "remote_host", "remote_port", "session_start_time", "slice_id", "sql_state_code", "stack_trace", "sub_tranx_id", "thread_id", "transaction_id", "user_name"] |
