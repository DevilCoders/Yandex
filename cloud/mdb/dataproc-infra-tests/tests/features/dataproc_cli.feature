Feature: Run CLI tasks on Dataproc cluster

  Background: Wait until internal api is ready
    Given Internal API is up and running
    Given we are working with standard Hadoop cluster
    Given username ubuntu


  @create
  Scenario: Create cluster
    Given we put ssh public keys to staging/mdb_infratest_public_keys
    When we run YC CLI retrying ResourceExhausted exception for "20 minutes"
    """
    yc dataproc cluster create
    --name cli_test_cluster
    --description "Initial description"
    --version "2.1"
    --ssh-public-keys-file staging/mdb_infratest_public_keys
    --services hdfs,yarn,spark
    --subcluster name=master,role=MASTERNODE,resource-preset=s2.small,disk-size=20,hosts-count=1,subnet-id={{ defaultSubnetId }}
    --subcluster name=data,role=DATANODE,resource-preset=s2.micro,disk-size=20,hosts-count=1,subnet-id={{ defaultSubnetId }}
    --service-account-id {{ serviceAccountId1 }}
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
    name: cli_test_cluster
    description: "Initial description"
    config:
      version_id: "2.1"
      hadoop:
        services:
          - HDFS
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
    service_account_id: {{ serviceAccountId1 }}
    bucket: {{ outputBucket1 }}
    """
    And cluster "cli_test_cluster" is up and running
    And all hosts of current cluster have service account {{ serviceAccountId1 }}
    And user-data attribute of metadata of all hosts of current cluster contains
    """
    s3_bucket: {{ outputBucket1 }}
    """
    And all 1 hosts of subcluster "master" have following labels
    """
    owner: {{ conf['user'] }}
    cluster_id: {{ cluster['id'] }}
    subcluster_id: {{ subclusters_id_by_name['master'] }}
    subcluster_role: masternode
    folder_id: {{ folder['folder_ext_id'] }}
    cloud_id: {{ folder['cloud_ext_id'] }}
    """
    And all 1 hosts of subcluster "data" have network-hdd boot disk of size 20 GB
    And all 1 hosts of subcluster "data" have 2 cores
    And all 1 hosts of subcluster "data" have following labels
    """
    owner: {{ conf['user'] }}
    cluster_id: {{ cluster['id'] }}
    subcluster_id: {{ subclusters_id_by_name['data'] }}
    subcluster_role: datanode
    folder_id: {{ folder['folder_ext_id'] }}
    cloud_id: {{ folder['cloud_ext_id'] }}
    """
    And all instances of cluster have a security_group {{ securityGroupId }}


  @cluster-update
  Scenario: Update name of the cluster
    Given cluster "cli_test_cluster" is up and running

    When we run YC CLI
    """
    yc dataproc cluster update cli_test_cluster --new-name cli_test_cluster_updated
    """
    Then YC CLI exit code is 0
    And cluster "cli_test_cluster_updated" is up and running

    When we run YC CLI
    """
    yc dataproc cluster update cli_test_cluster_updated --new-name cli_test_cluster
    """
    Then YC CLI exit code is 0
    And cluster "cli_test_cluster" is up and running


  @cluster-update
  Scenario: Update description of the cluster
    Given cluster "cli_test_cluster" is up and running

    When we run YC CLI
    """
    yc dataproc cluster update cli_test_cluster --description "Description updated"
    """
    Then YC CLI exit code is 0

    When we run YC CLI
    """
    yc dataproc cluster get cli_test_cluster
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    description: "Description updated"
    """


  @cluster-update
  Scenario: Update labels of the cluster
    Given cluster "cli_test_cluster" is up and running

    When we run YC CLI
    """
    yc dataproc cluster update cli_test_cluster --labels owner={{ conf['user'] }},foo1=bar1,foo2=bar2
    """
    Then YC CLI exit code is 0

    And all 1 hosts of subcluster "master" have following labels
    """
    owner: {{ conf['user'] }}
    foo1: bar1
    foo2: bar2
    cluster_id: {{ cluster['id'] }}
    subcluster_id: {{ subclusters_id_by_name['master'] }}
    subcluster_role: masternode
    folder_id: {{ folder['folder_ext_id'] }}
    cloud_id: {{ folder['cloud_ext_id'] }}
    """
    And all 1 hosts of subcluster "data" have following labels
    """
    owner: {{ conf['user'] }}
    foo1: bar1
    foo2: bar2
    cluster_id: {{ cluster['id'] }}
    subcluster_id: {{ subclusters_id_by_name['data'] }}
    subcluster_role: datanode
    folder_id: {{ folder['folder_ext_id'] }}
    cloud_id: {{ folder['cloud_ext_id'] }}
    """


  @cluster-update
  Scenario: Compute instance labels are updated correctly for existing hosts and set correctly for new hosts
    Given cluster "cli_test_cluster" is up and running
    # intentionally use api instead of cli, because we want to test modification of subclusters
    When we attempt to modify cluster "cli_test_cluster" with following parameters
    """
    {
      "configSpec": {
        "subclustersSpec": [
          {
            "id": "{{subclusters_id_by_name['master']}}",
            "name": "main"
          },
          {
            "id": "{{subclusters_id_by_name['data']}}",
            "resources": {
              "diskSize": {{ 24 * 2**30 }},
              "resourcePresetId": "s2.small"
            },
            "hostsCount": 2
          }
        ]
      },
      "labels": {
          "owner": "{{ conf['user'] }}",
          "foo3": "bar3",
          "foo4": "bar4"
      }
    }
    """
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And all 2 hosts of subcluster "data" have network-hdd boot disk of size 24 GB
    And all 2 hosts of subcluster "data" have 4 cores
    And all 2 hosts of subcluster "data" have following labels
    """
    owner: {{ conf['user'] }}
    foo3: bar3
    foo4: bar4
    cluster_id: {{ cluster['id'] }}
    subcluster_id: {{ subclusters_id_by_name['data'] }}
    subcluster_role: datanode
    folder_id: {{ folder['folder_ext_id'] }}
    cloud_id: {{ folder['cloud_ext_id'] }}
    """


  @cluster-update @sa_and_bucket
  Scenario: Try to set service account without role dataproc.agent
    Given cluster "cli_test_cluster" is up and running

    When we attempt to modify cluster "cli_test_cluster" with following parameters
    """
    {
      "serviceAccountId": "{{ serviceAccountId3 }}"
    }
    """
    Then response should have status 422 and body contains
    """
    {
      "code": 3,
      "message": "Service account {{ serviceAccountId3 }} should have role `dataproc.agent` for specified folder {{ folder['folder_ext_id'] }}"
    }
    """


  @cluster-update @sa_and_bucket
  Scenario: Update bucket and service account, SA has access to bucket
    Given cluster "cli_test_cluster" is up and running
    When we run YC CLI
    """
    yc dataproc cluster update cli_test_cluster --service-account-id {{ serviceAccountId2 }} --bucket {{ outputBucket2 }}
    """
    Then YC CLI exit code is 0
    And cluster "cli_test_cluster" is up and running
    And all hosts of current cluster have service account {{ serviceAccountId2 }}
    And user-data attribute of metadata of all hosts of current cluster contains
    """
    s3_bucket: {{ outputBucket2 }}
    """

  @subcluster
  Scenario: Create subcluster
    Given cluster "cli_test_cluster" is up and running

    When we run YC CLI retrying ResourceExhausted exception for "20 minutes"
    """
    yc dataproc subcluster create
    --cluster-name cli_test_cluster
    --name compute
    --role COMPUTENODE
    --resource-preset s2.micro
    --disk-type network-hdd
    --subnet-id {{ defaultSubnetId }}
    --disk-size 20
    --hosts-count 1
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    cluster_id: {{ cid }}
    name: compute
    role: COMPUTENODE
    resources:
      resource_preset_id: s2.micro
      disk_type_id: network-hdd
      disk_size: "{{ 20 * 2**30 }}"
    subnet_id: "{{ defaultSubnetId }}"
    hosts_count: "1"
    """
    And all 1 hosts of subcluster "compute" have network-hdd boot disk of size 20 GB
    And all 1 hosts of subcluster "compute" have 2 cores
    And all 1 hosts of subcluster "compute" have following labels
    """
    owner: {{ conf['user'] }}
    foo3: bar3
    foo4: bar4
    cluster_id: {{ cluster['id'] }}
    subcluster_id: {{ subclusters_id_by_name['compute'] }}
    subcluster_role: computenode
    folder_id: {{ folder['folder_ext_id'] }}
    cloud_id: {{ folder['cloud_ext_id'] }}
    """
    And all instances of cluster have a security_group {{ securityGroupId }}


  @subcluster
  Scenario: Update name of the subcluster
    Given cluster "cli_test_cluster" is up and running

    When we run YC CLI
    """
    yc dataproc subcluster update compute
    --new-name compute-updated
    --cluster-name cli_test_cluster
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    name: compute-updated
    """
    Then we wait no more than "1 minute" until cluster health is ALIVE

    When we run YC CLI
    """
    yc dataproc subcluster update compute-updated
    --new-name compute
    --cluster-name cli_test_cluster
    """
    Then YC CLI exit code is 0
    Then we wait no more than "1 minute" until cluster health is ALIVE
    And all instances of cluster have a security_group {{ securityGroupId }}


  @subcluster
  Scenario: Update disk size of host of the subcluster
    Given cluster "cli_test_cluster" is up and running

    When we run YC CLI retrying ResourceExhausted exception for "20 minutes"
    """
    yc dataproc subcluster update compute
    --disk-size 24
    --cluster-name cli_test_cluster
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    resources:
      resource_preset_id: s2.micro
      disk_type_id: network-hdd
      disk_size: "{{ 24 * 2**30 }}"
    """
    And all 1 hosts of subcluster "compute" have network-hdd boot disk of size 24 GB
    And all instances of cluster have a security_group {{ securityGroupId }}


  @subcluster
  Scenario: Update resource preset of hosts of the subcluster
    Given cluster "cli_test_cluster" is up and running

    When we run YC CLI retrying ResourceExhausted exception for "20 minutes"
    """
    yc dataproc subcluster update compute
    --resource-preset s2.small
    --cluster-name cli_test_cluster
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    resources:
      resource_preset_id: s2.small
      disk_type_id: network-hdd
      disk_size: "{{ 24 * 2**30 }}"
    """
    And all 1 hosts of subcluster "compute" have 4 cores
    And all instances of cluster have a security_group {{ securityGroupId }}


  @subcluster
  Scenario: Update hosts count of the subcluster
    Given cluster "cli_test_cluster" is up and running

    When we run YC CLI retrying ResourceExhausted exception for "20 minutes"
    """
    yc dataproc subcluster update compute
    --hosts-count 2
    --cluster-name cli_test_cluster
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    hosts_count: "2"
    """
    And all 2 hosts of subcluster "compute" have 4 cores
    And all 2 hosts of subcluster "compute" have network-hdd boot disk of size 24 GB
    And all 2 hosts of subcluster "compute" have following labels
    """
    owner: {{ conf['user'] }}
    foo3: bar3
    foo4: bar4
    cluster_id: {{ cluster['id'] }}
    subcluster_id: {{ subclusters_id_by_name['compute'] }}
    subcluster_role: computenode
    folder_id: {{ folder['folder_ext_id'] }}
    cloud_id: {{ folder['cloud_ext_id'] }}
    """
    And all instances of cluster have a security_group {{ securityGroupId }}


  @subcluster @security-groups
  Scenario: Remove security groups
    Given cluster "cli_test_cluster" is up and running
    When we run YC CLI
    """
    yc dataproc cluster update cli_test_cluster --security-group-ids=
    """
    Then YC CLI exit code is 0
    Then all instances haven't security_groups


  @subcluster @security-groups
  Scenario: Set security group for cluster without security groups
    Given cluster "cli_test_cluster" is up and running
    When we run YC CLI
    """
    yc dataproc cluster update cli_test_cluster --security-group-ids {{ securityGroupId }}
    """
    Then all instances of cluster have a security_group {{ securityGroupId }}


  @subcluster
  Scenario: Delete subcluster
    Given cluster "cli_test_cluster" is up and running

    When we run YC CLI
    """
    yc dataproc subcluster delete compute --cluster-name cli_test_cluster
    """
    Then YC CLI exit code is 0


  @autoscaling-subcluster @trunk
  Scenario: Autoscaling compute subcluster is successfully created
    Given cluster "cli_test_cluster" is up and running
    When we run YC CLI
    """
    yc dataproc subcluster create
    --name Compute-autoscaling
    --role computenode
    --resource-preset s2.micro
    --hosts-count 1
    --cluster-name cli_test_cluster
    --subnet-id {{ defaultSubnetId }}
    --max-hosts-count 2
    --cpu-utilization-target 65
    """
    Then YC CLI exit code is 0
    And all 1 hosts of subcluster "Compute-autoscaling" have 2 cores
    And all instances of cluster have a security_group {{ securityGroupId }}


  @autoscaling-subcluster @trunk
  Scenario: Autoscaling compute subcluster is successfully modified
    Given cluster "cli_test_cluster" is up and running
    When we run YC CLI
    """
    yc dataproc subcluster update
    --name Compute-autoscaling
    --cluster-name cli_test_cluster
    --hosts-count 2
    --max-hosts-count 4
    --cpu-utilization-target 0
    """
    Then YC CLI exit code is 0
    And all 2 hosts of subcluster "Compute-autoscaling" have 2 cores
    And all instances of cluster have a security_group {{ securityGroupId }}


  @autoscaling-subcluster @trunk
  Scenario: Autoscaling compute subcluster is successfully deleted
    Given cluster "cli_test_cluster" is up and running
    When subcluster "Compute-autoscaling" instance group id is loaded into context
    When we run YC CLI
    """
    yc dataproc subcluster delete Compute-autoscaling --cluster-name cli_test_cluster
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


  Scenario: Modify cluster when job is running
    Given cluster "cli_test_cluster" is up and running

    # Create long running job (pi calc for ~8 minutes).
    # Deliberately run job via API in order not to wait for completion.
    When we run YC CLI
    """
    yc dataproc job create-spark
    --name "calculate PI"
    --main-jar-file-uri file:///usr/lib/spark/examples/jars/spark-examples.jar
    --main-class org.apache.spark.examples.SparkPi
    --args 10000
    --async
    """
    Then YC CLI exit code is 0

    # Run some cluster modification operation while job is running.
    # Deliberately modify cluster via API in order to control running time.
    When we attempt to modify cluster "cli_test_cluster" with following parameters
    """
    {
      "labels": {
          "owner": "{{ conf['user'] }}",
          "foo1": "bar1"
      }
    }
    """
    Then response should have status 200
    And generated task is finished within "4 minutes"


  Scenario: Unable to modify cluster if it is not ALIVE
    Given cluster "cli_test_cluster" is up and running
    When we execute "sudo service hadoop-yarn@resourcemanager stop" on master node
    Then we wait no more than "1 minute" until cluster health is DEAD
    When we run YC CLI
    """
    yc dataproc subcluster update data
    --disk-size 24
    --cluster-name cli_test_cluster
    """
    Then YC CLI exit code is 1
    And YC CLI error contains
    """
    Unable to modify cluster while its health status is Dead: some critical services are Dead: YARN is Dead (service is not available)
    """
    When we execute "sudo service hadoop-yarn@resourcemanager start" on master node
    Then we wait no more than "1 minute" until cluster health is ALIVE


  @delete
  Scenario: Delete cluster
    When we run YC CLI
    """
    yc dataproc cluster delete cli_test_cluster
    """
    Then YC CLI exit code is 0
