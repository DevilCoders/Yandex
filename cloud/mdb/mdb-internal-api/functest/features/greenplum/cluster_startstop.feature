@greenplum
@grpc_api
Feature: Greenplum-start-stop

  Background:
    Given default headers
    When we add default feature flag "MDB_GREENPLUM_CLUSTER"
    When we add default feature flag "MDB_GREENPLUM_ALLOW_LOW_MEM"
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "folder_id": "folder1",
            "environment": "PRESTABLE",
            "name": "test",
            "description": "test cluster",
            "labels": {
                "foo": "bar"
            },
            "network_id": "network1",
            "config": {
                "version": "6.19",
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
            "segment_in_host": 3,
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

  Scenario: Cluster stop and start works
    When we "Stop" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
    Then we get gRPC response with body
        """
        {
            "created_by": "user",
            "description": "Stop Greenplum cluster",
            "done": false,
            "id": "worker_task_id2",
            "metadata": {
              "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.StopClusterMetadata",
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
    And "worker_task_id2" acquired and finished by worker
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
    Then we get gRPC response with body
        """
        {
            "description": "test cluster",
            "environment": "PRESTABLE",
            "folder_id": "folder1",
            "id": "cid1",
            "name": "test",
            "network_id": "network1",
            "status": "STOPPED"
        }
        """
    When we "Start" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
    Then we get gRPC response with body
        """
        {
            "created_by": "user",
            "description": "Start Greenplum cluster",
            "done": false,
            "id": "worker_task_id3",
            "metadata": {
              "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.StartClusterMetadata",
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
    And "worker_task_id3" acquired and finished by worker
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
    Then we get gRPC response with body
        """
        {
            "description": "test cluster",
            "environment": "PRESTABLE",
            "folder_id": "folder1",
            "id": "cid1",
            "name": "test",
            "network_id": "network1",
            "status": "RUNNING"
        }
        """

  Scenario: Cluster stop and start checks works
    When we "Start" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "operation is not allowed in current cluster status"
    When we "Stop" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
    Then we get gRPC response OK
    And "worker_task_id2" acquired and finished by worker
    When we "Stop" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "operation is not allowed in current cluster status"
