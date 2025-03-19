Feature: Dataproc logging

  Background: Wait until internal api is ready
    Given Internal API is up and running
    Given we are working with autoscaling Hadoop cluster
    Given username ubuntu

  @create
  Scenario: Dataproc cluster created successfully
    Given we put ssh public keys to staging/mdb_infratest_public_keys
    When we run YC CLI retrying ResourceExhausted exception for "20 minutes"
    """
    yc dataproc cluster create
    --name test_cluster
    --version "2.1"
    --ssh-public-keys-file staging/mdb_infratest_public_keys
    --services yarn,spark
    --subcluster name=driver,role=MASTERNODE,resource-preset=s2.micro,disk-size=20,hosts-count=1,subnet-id={{ defaultSubnetId }}
    --subcluster name=executors,role=COMPUTENODE,resource-preset=s2.micro,disk-size=20,hosts-count=2,subnet-id={{ defaultSubnetId }}
    --service-account-id {{ serviceAccountId2 }}
    --log-group-id {{ logGroupId }}
    --bucket {{ outputBucket1 }}
    --zone {{cluster_config['zoneId']}}
    --labels owner={{ conf['user'] }}
    --property dataproc:version={{ agent_version }}
    --property dataproc:repo={{ agent_repo }}
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    name: test_cluster
    config:
      version_id: "2.1"
      hadoop:
        services:
          - SPARK
          - YARN
        ssh_public_keys:
          - {{cluster_config['configSpec']['hadoop']['sshPublicKeys'][0]}}
          - {{cluster_config['configSpec']['hadoop']['sshPublicKeys'][1]}}
        properties:
          "dataproc:version": "{{ agent_version }}"
          "dataproc:repo": "{{ agent_repo }}"
    health: ALIVE
    zone_id: {{cluster_config['zoneId']}}
    service_account_id: {{ serviceAccountId2 }}
    bucket: {{ outputBucket1 }}
    """
    And cluster "test_cluster" is up and running


  @check_logging @skip
  Scenario: Logs are sent to logging service
    Given cluster "test_cluster" is up and running
    When we run YC CLI
    """
    yc logging read
    --group-id {{ logGroupId }}
    --resource-types dataproc.cluster
    --resource-ids {{ cid }}
    --format yaml
    --filter 'log_type:syslog'
    --limit 1
    """
    Then YC CLI exit code is 0
    And YC CLI response key "json_payload" contains
    """
    "log_type": "syslog"
    """
    And YC CLI response key "resource" contains
    """
    "id": {{ cid }}
    """


  @check_log_group @skip
  Scenario: Using wrong log group fails
    Given cluster "test_cluster" is up and running
    When we run YC CLI
    """
    yc dataproc cluster update
    --name test_cluster
    --log-group-id nonexistent
    """
    Then YC CLI exit code is 1
    And YC CLI error contains
    """
    ERROR: rpc error: code = InvalidArgument desc = Can not find log group nonexistent.
    """


  @check_auth_get_log_group @skip
  Scenario: Using wrong log group fails
    When we run YC CLI
    """
    yc dataproc cluster update
    --name test_cluster
    --log-group-id {{ wrongLogGroupId }}
    """
    Then YC CLI exit code is 1
    And YC CLI error contains
    """
    Can not find log group {{ wrongLogGroupId }}. Or service account {{ serviceAccountId2 }} does not have permission to get log group {{ wrongLogGroupId }}. Grant role "logging.writer" to service account {{ serviceAccountId2 }} for the folder of this log group.
    """


  @check_auth_write_log_group @skip
  Scenario: Using service account without write permission for the log group fails
    When we run YC CLI
    """
    yc dataproc cluster update
    --name test_cluster
    --service-account-id {{ serviceAccountId1 }}
    """
    Then YC CLI exit code is 1
    And YC CLI error contains
    """
    Service account {{ serviceAccountId1 }} does not have permission to write to log group {{ logGroupId }}. Grant role "logging.writer" to service account {{ serviceAccountId1 }} for folder {{ folderId }}
    """

  @change_to_default @skip
  Scenario: Change to default log group id
    When we run YC CLI
    """
    yc dataproc cluster update
    --name test_cluster
    --log-group-id ''
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    name: test_cluster
    config:
      version_id: "2.1"
      hadoop:
        services:
          - SPARK
          - YARN
        ssh_public_keys:
          - {{cluster_config['configSpec']['hadoop']['sshPublicKeys'][0]}}
          - {{cluster_config['configSpec']['hadoop']['sshPublicKeys'][1]}}
        properties:
          "dataproc:version": "{{ agent_version }}"
          "dataproc:repo": "{{ agent_repo }}"
    health: ALIVE
    zone_id: {{cluster_config['zoneId']}}
    service_account_id: {{ serviceAccountId2 }}
    bucket: {{ outputBucket1 }}
    """
    And cluster "test_cluster" is up and running


  @check_invalid_auth
  Scenario: User without "logging.reader" role can not read job logs
    Given cluster "test_cluster" is up and running
    When we run YC CLI
    """
    yc dataproc job create-spark
    --name "calculate PI"
    --main-jar-file-uri file:///usr/lib/spark/examples/jars/spark-examples.jar
    --main-class org.apache.spark.examples.SparkPi
    --args 10
    """
    Then YC CLI exit code is 0
    And dataproc job started via YC CLI is DONE
    When we run YC CLI
    """
    yc dataproc job log {{ job_id }} --profile dataproc-user-sa
    """
    Then YC CLI exit code is 1
    And YC CLI error contains
    """
    Can not find log group {{ logGroupId }}. Or you do not have permission to read from log group {{ logGroupId }}. Make sure you have role "logging.reader" for the folder of this log group.
    """

  @delete
  Scenario: Cluster is successfully deleted
    Given cluster "test_cluster" exists
    When we run YC CLI
    """
    yc dataproc cluster delete test_cluster
    """
    Then YC CLI exit code is 0
