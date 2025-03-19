Feature: Modify Data Proc cluster validation

  Background:
    Given default headers
    When we POST "/mdb/hadoop/1.0/clusters?folderId=folder1" with data
    """
    {
        "folderId": "folder1",
        "name": "test",
        "configSpec": {
            "versionId": "1.4",
            "hadoop": {
                "sshPublicKeys": ["ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAtkGK0D8c/5Yk7NCeZ36fOvsmrSLvAoK6PSd6lzhRvZ"]
            },
            "subclustersSpec": [
                {
                    "name": "main",
                    "role": "MASTERNODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 16106127360
                    },
                    "subnetId": "network1-myt"
                },
                {
                    "name": "data",
                    "role": "DATANODE",
                    "resources": {
                        "resourcePresetId": "s1.compute.1",
                        "diskTypeId": "network-ssd",
                        "diskSize": 16106127360
                    },
                    "hostsCount": 5,
                    "subnetId": "network1-myt"
                }
            ]
        },
        "description": "test cluster",
        "zoneId": "myt",
        "serviceAccountId": "service_account_1"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create Data Proc cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.dataproc.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker
    When we run query
    """
    UPDATE dbaas.subclusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """

  Scenario: Deleting masternode subcluster does not work
  When we DELETE "/mdb/hadoop/1.0/clusters/cid1/subclusters/subcid1"
  Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Only deleting computenodes is allowed"
    }
    """

  Scenario: Deleting datanode subcluster does not work
  When we DELETE "/mdb/hadoop/1.0/clusters/cid1/subclusters/subcid2"
  Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Only deleting computenodes is allowed"
    }
    """

  Scenario: Changing cluster's service account to unauthorized service account fails
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "serviceAccountId": "service_account_without_permission"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Service account service_account_without_permission should have role `dataproc.agent` for specified folder folder1"
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "serviceAccountId": "service_account_1"
    }
    """

  Scenario: Changing cluster's service account to service account that the user does not have permission to use
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1" with data
    """
    {
        "serviceAccountId": "service_account_3"
    }
    """
    Then we get response with status 403 and body contains
    """
    {
        "code": 7,
        "message": "You do not have permission to access the requested service account or service account does not exist"
    }
    """
    When we GET "/mdb/hadoop/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "serviceAccountId": "service_account_1"
    }
    """


  Scenario: Subcluster add with nonuniq name fails
  When we POST "/mdb/hadoop/1.0/clusters/cid1/subclusters" with data
  """
    {
      "name": "data",
      "hostsCount": 3,
      "resources": {
          "diskSize": 16106127360,
          "diskTypeId": "network-ssd",
          "resourcePresetId": "s1.compute.1"
      },
      "role": "COMPUTENODE",
      "subnetId": "network1-myt"
    }
  """
  Then we get response with status 422 and body contains
  """
    {
        "code": 3,
        "message": "Subclusters names must be unique"
    }
  """

  Scenario: Changing disk type id fails
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1/subclusters/subcid2" with data
    """
    {
      "resources": {
          "diskTypeId": "local-ssd"
      }
    }
    """
    Then we get response with status 501 and body contains
    """
    {
        "code": 12,
        "message": "Changing disk_type_id for Data Proc subcluster is not supported yet"
    }
    """

  Scenario: Decreasing  subcluster disk size fails
    When we PATCH "/mdb/hadoop/1.0/clusters/cid1/subclusters/subcid2" with data
    """
    {
        "resources": {
            "diskSize": 15032385536
        }
    }
    """
    Then we get response with status 501 and body contains
    """
    {
        "code": 12,
        "message": "Decreasing disk size for Data Proc subcluster nodes is not supported yet"
    }
    """
