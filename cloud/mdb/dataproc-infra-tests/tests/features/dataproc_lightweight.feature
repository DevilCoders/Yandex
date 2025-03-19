@lightweight
Feature: Lightweight Data Proc with Spark works smoothly

  Background: Wait until internal api is ready
    Given Internal API is up and running
    Given we are working with autoscaling Hadoop cluster
    Given username ubuntu

  @create
  Scenario: Lightweight cluster created successfully
    Given we put ssh public keys to staging/mdb_infratest_public_keys
    When we run YC CLI retrying ResourceExhausted exception for "10 minutes"
    """
    yc dataproc cluster create
    --name test_cluster
    --version "2.1"
    --ssh-public-keys-file staging/mdb_infratest_public_keys
    --services yarn,spark,livy
    --subcluster name=driver,role=MASTERNODE,resource-preset=s2.micro,disk-size=20,hosts-count=1,subnet-id={{ defaultSubnetId }}
    --subcluster name=executors,role=COMPUTENODE,preemptible=true,resource-preset=s2.micro,disk-size=20,hosts-count=1,max-hosts-count=2,subnet-id={{ defaultSubnetId }},stabilization-duration=4m
    --service-account-id {{ serviceAccountId2 }}
    --bucket {{ outputBucket1 }}
    --zone {{cluster_config['zoneId']}}
    --labels owner={{ conf['user'] }}
    --log-group-id {{ logGroupId }}
    --property yarn:yarn.nm.liveness-monitor.expiry-interval-ms=15000
    --property yarn:yarn.log-aggregation-enable=false
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
          - LIVY
          - SPARK
          - YARN
        ssh_public_keys:
          - {{cluster_config['configSpec']['hadoop']['sshPublicKeys'][0]}}
          - {{cluster_config['configSpec']['hadoop']['sshPublicKeys'][1]}}
        properties:
          "dataproc:version": "{{ agent_version }}"
          "dataproc:repo": "{{ agent_repo }}"
          "yarn:yarn.nm.liveness-monitor.expiry-interval-ms": "15000"
          "yarn:yarn.log-aggregation-enable": "false"
    health: ALIVE
    zone_id: {{cluster_config['zoneId']}}
    service_account_id: {{ serviceAccountId2 }}
    bucket: {{ outputBucket1 }}
    """
    And cluster "test_cluster" is up and running


  Scenario: Lightweight cluster autoscales under spark load
    Given cluster "test_cluster" is up and running
    When subcluster "executors" instance group id is loaded into context
    And we run YC CLI
    """
    yc compute instance-group get {{ instance_group_id }}
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    managed_instances_state:
      target_size: "1"
      running_actual_count: "1"
    """
    When we run YC CLI
    """
    yc dataproc job create-spark
    --name "heavy Pi calculation"
    --main-jar-file-uri file:///usr/lib/spark/examples/jars/spark-examples.jar
    --main-class org.apache.spark.examples.SparkPi
    --args 10000
    --async
    """
    Then YC CLI exit code is 0
    When we run YC CLI
    """
    yc compute instance-group get {{ instance_group_id }}
    """
    Then YC CLI exit code is 0
    And periodically run previous YC CLI command for "8 minutes" until response contains
    """
    managed_instances_state:
      target_size: "2"
      running_actual_count: "2"
    """
    When we run YC CLI
    """
    yc compute instance-group get {{ instance_group_id }}
    """
    Then YC CLI exit code is 0
    Then periodically run previous YC CLI command for "8 minutes" until response contains
    """
    managed_instances_state:
      target_size: "1"
      running_actual_count: "1"
    """

  @delete
  Scenario: Cluster is successfully deleted
    Given cluster "test_cluster" exists
    When we run YC CLI
    """
    yc dataproc cluster delete test_cluster
    """
    Then YC CLI exit code is 0
