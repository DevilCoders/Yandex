Feature: Dataproc autoscaling

  Background: Wait until internal api is ready
    Given Internal API is up and running
    Given we are working with autoscaling Hadoop cluster
    Given username ubuntu

  @create
  Scenario: Autoscaling cluster created successfully
    Given we put ssh public keys to staging/mdb_infratest_public_keys
    When we run YC CLI retrying ResourceExhausted exception for "20 minutes"
    """
    yc dataproc cluster create
    --name test_cluster
    --description "Initial description"
    --ssh-public-keys-file staging/mdb_infratest_public_keys
    --services hdfs,yarn,mapreduce,tez,hive,spark
    --subcluster name=master,role=MASTERNODE,resource-preset=s2.micro,disk-size=20,hosts-count=1,subnet-id={{ defaultSubnetId }}
    --subcluster name=data,role=DATANODE,resource-preset=s2.micro,disk-size=20,hosts-count=1,subnet-id={{ defaultSubnetId }}
    --subcluster name=Compute-autoscaling,role=COMPUTENODE,preemptible=true,resource-preset=s2.micro,disk-size=20,hosts-count=0,max-hosts-count=2,subnet-id={{ defaultSubnetId }},stabilization-duration=4m
    --service-account-id {{ serviceAccountId2 }}
    --bucket {{ outputBucket1 }}
    --zone {{cluster_config['zoneId']}}
    --labels owner={{ conf['user'] }}
    --log-group-id {{ logGroupId }}
    --security-group-ids {{ securityGroupId }}
    --property yarn:yarn.nm.liveness-monitor.expiry-interval-ms=15000
    --property yarn:yarn.log-aggregation-enable=false
    --property dataproc:version={{ agent_version }}
    --property dataproc:repo={{ agent_repo }}
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    name: test_cluster
    description: "Initial description"
    config:
      version_id: "{{ get_default_dataproc_version_prefix() }}"
      hadoop:
        services:
          - HDFS
          - HIVE
          - MAPREDUCE
          - SPARK
          - TEZ
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
    And all instances of cluster have a security_group {{ securityGroupId }}


  @delete_subcluster @skip
  Scenario: Autoscaling compute subcluster is successfully deleted
    Given cluster "test_cluster" is up and running
    When subcluster "Compute-autoscaling" instance group id is loaded into context
    When we run YC CLI
    """
    yc dataproc subcluster delete Compute-autoscaling --cluster-name test_cluster
    """
    Then YC CLI exit code is 0
    When we run YC CLI
    """
    yc compute instance-group get {{ instance_group_id }}
    """
      Then YC CLI exit code is 1
      And YC CLI error contains
    """
    ERROR: instance-group with id or name "{{ instance_group_id }}" not found
    """


  @create_subcluster @skip
  Scenario: Autoscaling burstable compute subcluster is failed to be created
    Given cluster "test_cluster" is up and running
    When we run YC CLI
    """
    yc dataproc subcluster create
    --name Compute-autoscaling
    --role computenode
    --hosts-count 1
    --resource-preset b2.small
    --cluster-name test_cluster
    --subnet-id {{ defaultSubnetId }}
    --max-hosts-count 1
    """
    Then YC CLI exit code is 1
    And YC CLI error contains
    """
    ERROR: rpc error: code = InvalidArgument desc = Burstable instances are not supported for instance groups. Choose other preset.
    """


  @create_subcluster @skip
  Scenario: Autoscaling compute subcluster is successfully created
    Given cluster "test_cluster" is up and running
    When we run YC CLI
    """
    yc dataproc subcluster create
    --name Compute-autoscaling
    --role computenode
    --hosts-count 1
    --cluster-name test_cluster
    --subnet-id {{ defaultSubnetId }}
    --max-hosts-count 2
    --cpu-utilization-target 65
    """
    Then YC CLI exit code is 0


  @modify_subcluster @skip
  Scenario: Autoscaling compute subcluster is successfully modified
    Given cluster "test_cluster" is up and running
    When we run YC CLI
    """
    yc dataproc subcluster update
    --name Compute-autoscaling
    --cluster-name test_cluster
    --max-hosts-count 4
    --disk-size 32
    --cpu-utilization-target 0
    """
    Then YC CLI exit code is 0
    When subcluster "Compute-autoscaling" instance group id is loaded into context
    When we run YC CLI
    """
    yc compute instance-group get {{ instance_group_id }}
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    scale_policy:
      auto_scale:
        min_zone_size: "1"
        max_size: "4"
        measurement_duration: 60s
        warmup_duration: 60s
        stabilization_duration: 120s
        initial_size: "1"
        custom_rules:
        - rule_type: WORKLOAD
          metric_type: GAUGE
          metric_name: yarn.cluster.containersPending
          labels:
            resource_id: {{ cid }}
            resource_type: cluster
          target: 2
    """

  @check_references
  Scenario: Deleting cluster instance groups using compute api fails
    Given cluster "test_cluster" is up and running
    When subcluster "Compute-autoscaling" instance group id is loaded into context
    When we run YC CLI
    """
    yc compute instance-group delete {{ instance_group_id }}
    """
    Then YC CLI exit code is 1
    And YC CLI error contains
    """
    ERROR: rpc error: code = FailedPrecondition desc = The entity management is only allowed to: dataproc.subcluster
    """

  @check_references @skip
  Scenario: Deleting cluster instances using compute api fails
    Given cluster "test_cluster" is up and running
    When masternode host is loaded into context
    When we run YC CLI
    """
    yc compute instance delete {{ masternode_instance_id }}
    """
    Then YC CLI exit code is 1
    And YC CLI error contains
    """
    ERROR: rpc error: code = FailedPrecondition desc = target object is managed by
    """


  @precheck
  Scenario: Subcluster nodes number is minimal when no jobs are running
    Given cluster "test_cluster" is up and running
    When subcluster "Compute-autoscaling" instance group id is loaded into context
    And we run YC CLI
    """
    yc compute instance-group get {{ instance_group_id }}
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    managed_instances_state: {}
    """


  @run @hive
  Scenario: Hive job allows to execute sql script file and pass variables to it
    Given cluster "test_cluster" is up and running
    When we run YC CLI "6" times
    """
    yc dataproc job create-hive
    --name "find total population of cities in Russia"
    --query-file-uri s3a://dataproc-e2e/jobs/sources/hive-001/main.sql
    --script-variables CITIES_URI=s3a://dataproc-e2e/jobs/sources/data/hive-cities/,COUNTRY_CODE=RU
    --async
    """
    Then YC CLI exit code is 0


  @postcheck
  Scenario: Subcluster nodes number is increased after load is set
    Given cluster "test_cluster" is up and running
    When subcluster "Compute-autoscaling" instance group id is loaded into context
    And we run YC CLI
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


  @scale_down
  Scenario: Subcluster is scaled down after load is taken out
    Given cluster "test_cluster" is up and running
    When subcluster "Compute-autoscaling" instance group id is loaded into context
    And we run YC CLI
    """
    yc compute instance-group get {{ instance_group_id }}
    """
    Then YC CLI exit code is 0
    Then periodically run previous YC CLI command for "8 minutes" until response contains
    """
    managed_instances_state: {}
    """


  @stop @skip
  Scenario: Cluster is successfully stopped
    Given cluster "test_cluster" is up and running
    When subcluster "Compute-autoscaling" instance group id is loaded into context
    When we run YC CLI
    """
    yc dataproc cluster stop test_cluster
    """
    Then YC CLI exit code is 0
    When we run YC CLI
    """
    yc compute instance-group get {{ instance_group_id }}
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    status: STOPPED
    """


  @start @skip
  Scenario: Cluster is successfully started
    Given cluster "test_cluster" exists
    When subcluster "Compute-autoscaling" instance group id is loaded into context
    When we run YC CLI
    """
    yc dataproc cluster start test_cluster
    """
    Then YC CLI exit code is 0
    When we run YC CLI
    """
    yc compute instance-group get {{ instance_group_id }}
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    status: ACTIVE
    """


  @delete
  Scenario: Cluster is successfully deleted
    Given cluster "test_cluster" exists
    When we run YC CLI
    """
    yc dataproc cluster delete test_cluster
    """
    Then YC CLI exit code is 0
