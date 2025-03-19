Feature: Run CLI tasks on Dataproc cluster

  Background: Wait until internal api is ready
    Given we are working with hadoop cluster

  @create
  Scenario: Create cluster
    When we run YC CLI with "20 minutes" timeout
    """
    yc dataproc cluster create
    --name cli_test_cluster
    --description "Initial description"
    --ssh-public-keys-file {{ test_config.dataproc.ssh_key.public_path }}
    --services hdfs,yarn,mapreduce,tez,hive,spark
    --subcluster name=master,role=MASTERNODE,resource-preset=s2.small,disk-size=20,hosts-count=1,subnet-id={{ test_config.dataproc.default_subnet_id }}
    --subcluster name=data,role=DATANODE,resource-preset=s2.micro,disk-size=20,hosts-count=1,subnet-id={{ test_config.dataproc.default_subnet_id }}
    --service-account-id {{ test_config.dataproc.agent_sa_id }}
    --bucket {{ test_config.dataproc.output_bucket }}
    --zone {{ test_config.dataproc.zone }}
    --labels owner={{ test_config.cloud_resources_owner }}
    --log-group-id {{ test_config.dataproc.log_group_id }}
    --property yarn:yarn.nm.liveness-monitor.expiry-interval-ms=15000
    --property yarn:yarn.log-aggregation-enable=false
    --property dataproc:version={{ test_config.dataproc.agent_version }}
    --property dataproc:repo={{ test_config.dataproc.agent_repo }}
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    name: cli_test_cluster
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
          - {{ test_config.dataproc.ssh_key.public }}
        properties:
          "dataproc:version": "{{ test_config.dataproc.agent_version }}"
          "dataproc:repo": "{{ test_config.dataproc.agent_repo }}"
          "yarn:yarn.nm.liveness-monitor.expiry-interval-ms": "15000"
          "yarn:yarn.log-aggregation-enable": "false"
    health: ALIVE
    zone_id: {{ test_config.dataproc.zone }}
    service_account_id: {{ test_config.dataproc.agent_sa_id }}
    bucket: {{ test_config.dataproc.output_bucket }}
    """
    And cluster "cli_test_cluster" is up and running
    And all hosts of current cluster have service account {{ test_config.dataproc.agent_sa_id }}
    And user-data attribute of metadata of all hosts of current cluster contains
    """
    s3_bucket: {{ test_config.dataproc.output_bucket }}
    """
    And all 1 hosts of subcluster "master" have following labels
    """
    owner: {{ test_config.cloud_resources_owner }}
    cluster_id: {{ cid }}
    subcluster_id: {{ subcluster_id_by_name['master'] }}
    subcluster_role: masternode
    folder_id: {{ test_config.folder_id }}
    cloud_id: {{ test_config.cloud_id }}
    """
    And all 1 hosts of subcluster "data" have network-hdd boot disk of size 20 GB
    And all 1 hosts of subcluster "data" have 2 cores
    And all 1 hosts of subcluster "data" have following labels
    """
    owner: {{ test_config.cloud_resources_owner }}
    cluster_id: {{ cid }}
    subcluster_id: {{ subcluster_id_by_name['data'] }}
    subcluster_role: datanode
    folder_id: {{ test_config.folder_id }}
    cloud_id: {{ test_config.cloud_id }}
    """


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
    yc dataproc cluster update cli_test_cluster --labels owner={{ test_config.cloud_resources_owner }},foo1=bar1,foo2=bar2
    """
    Then YC CLI exit code is 0

    And all 1 hosts of subcluster "master" have following labels
    """
    owner: {{ test_config.cloud_resources_owner }}
    foo1: bar1
    foo2: bar2
    cluster_id: {{ cid }}
    subcluster_id: {{ subcluster_id_by_name['master'] }}
    subcluster_role: masternode
    folder_id: {{ test_config.folder_id }}
    cloud_id: {{ test_config.cloud_id }}
    """
    And all 1 hosts of subcluster "data" have following labels
    """
    owner: {{ test_config.cloud_resources_owner }}
    foo1: bar1
    foo2: bar2
    cluster_id: {{ cid }}
    subcluster_id: {{ subcluster_id_by_name['data'] }}
    subcluster_role: datanode
    folder_id: {{ test_config.folder_id }}
    cloud_id: {{ test_config.cloud_id }}
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
            "id": "{{ subcluster_id_by_name['master'] }}",
            "name": "main"
          },
          {
            "id": "{{ subcluster_id_by_name['data'] }}",
            "resources": {
              "diskSize": {{ 24 * 2**30 }},
              "resourcePresetId": "s2.small"
            },
            "hostsCount": 2
          }
        ]
      },
      "labels": {
          "owner": "{{ test_config.cloud_resources_owner }}",
          "foo3": "bar3",
          "foo4": "bar4"
      }
    }
    """
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And cluster "cli_test_cluster" is up and running
    And all 2 hosts of subcluster "data" have network-hdd boot disk of size 24 GB
    And all 2 hosts of subcluster "data" have 4 cores
    And all 2 hosts of subcluster "data" have following labels
    """
    owner: {{ test_config.cloud_resources_owner }}
    foo3: bar3
    foo4: bar4
    cluster_id: {{ cid }}
    subcluster_id: {{ subcluster_id_by_name['data'] }}
    subcluster_role: datanode
    folder_id: {{ test_config.folder_id }}
    cloud_id: {{ test_config.cloud_id }}
    """


  @cluster-update @sa_and_bucket
  Scenario: Try to set service account without role dataproc.agent
    Given cluster "cli_test_cluster" is up and running

    When we attempt to modify cluster "cli_test_cluster" with following parameters
    """
    {
      "serviceAccountId": "{{ test_config.dataproc.sa_without_role }}"
    }
    """
    Then response should have status 422 and body contains
    """
    {
      "code": 3,
      "message": "Service account {{ test_config.dataproc.sa_without_role }} should have role `dataproc.agent` for specified folder {{ test_config.folder_id }}"
    }
    """


  @cluster-update @sa_and_bucket
  Scenario: Update bucket and service account, SA has access to bucket
    Given cluster "cli_test_cluster" is up and running
    When we run YC CLI
    """
    yc dataproc cluster update cli_test_cluster --service-account-id {{ test_config.dataproc.agent_sa_2_id }} --bucket {{ test_config.dataproc.output_bucket_2 }}
    """
    Then YC CLI exit code is 0
    And cluster "cli_test_cluster" is up and running
    And all hosts of current cluster have service account {{ test_config.dataproc.agent_sa_2_id }}
    And user-data attribute of metadata of all hosts of current cluster contains
    """
    s3_bucket: {{ test_config.dataproc.output_bucket_2 }}
    """
    # need to wait until agent fetches updated metadata (currently fetch interval is 3 seconds)
    And we wait for "10 seconds"
    When we run YC CLI
    """
    yc dataproc job create-mapreduce
    --name "calculate PI"
    --main-class org.apache.hadoop.examples.QuasiMonteCarlo
    --args 1 --args 1000
    """
    Then YC CLI exit code is 0
    And dataproc job started via YC CLI is DONE
    And job driver output contains
    """
    Estimated value of Pi is
    """


  @subcluster
  Scenario: Create subcluster
    Given cluster "cli_test_cluster" is up and running

    When we run YC CLI with "20 minutes" timeout
    """
    yc dataproc subcluster create
    --cluster-name cli_test_cluster
    --name compute
    --role COMPUTENODE
    --resource-preset s2.micro
    --disk-type network-hdd
    --subnet-id {{ test_config.dataproc.default_subnet_id }}
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
    subnet_id: "{{ test_config.dataproc.default_subnet_id }}"
    hosts_count: "1"
    """
    And cluster "cli_test_cluster" is up and running
    And all 1 hosts of subcluster "compute" have network-hdd boot disk of size 20 GB
    And all 1 hosts of subcluster "compute" have 2 cores
    And all 1 hosts of subcluster "compute" have following labels
    """
    owner: {{ test_config.cloud_resources_owner }}
    foo3: bar3
    foo4: bar4
    cluster_id: {{ cid }}
    subcluster_id: {{ subcluster_id_by_name['compute'] }}
    subcluster_role: computenode
    folder_id: {{ test_config.folder_id }}
    cloud_id: {{ test_config.cloud_id }}
    """


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


  @subcluster
  Scenario: Update disk size of host of the subcluster
    Given cluster "cli_test_cluster" is up and running

    When we run YC CLI with "20 minutes" timeout
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
    And cluster "cli_test_cluster" is up and running
    And all 1 hosts of subcluster "compute" have network-hdd boot disk of size 24 GB


  @subcluster
  Scenario: Update resource preset of hosts of the subcluster
    Given cluster "cli_test_cluster" is up and running

    When we run YC CLI with "20 minutes" timeout
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
    And cluster "cli_test_cluster" is up and running
    And all 1 hosts of subcluster "compute" have 4 cores


  @subcluster
  Scenario: Update hosts count of the subcluster
    Given cluster "cli_test_cluster" is up and running

    When we run YC CLI with "20 minutes" timeout
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
    And cluster "cli_test_cluster" is up and running
    And all 2 hosts of subcluster "compute" have 4 cores
    And all 2 hosts of subcluster "compute" have network-hdd boot disk of size 24 GB
    And all 2 hosts of subcluster "compute" have following labels
    """
    owner: {{ test_config.cloud_resources_owner }}
    foo3: bar3
    foo4: bar4
    cluster_id: {{ cid }}
    subcluster_id: {{ subcluster_id_by_name['compute'] }}
    subcluster_role: computenode
    folder_id: {{ test_config.folder_id }}
    cloud_id: {{ test_config.cloud_id }}
    """


  @subcluster
  Scenario: Delete subcluster
    Given cluster "cli_test_cluster" is up and running

    When we run YC CLI
    """
    yc dataproc subcluster delete compute --cluster-name cli_test_cluster
    """
    Then YC CLI exit code is 0


  Scenario: Modify cluster when job is running
    Given cluster "cli_test_cluster" is up and running

    # Create long running job (sleep for 10 minutes).
    # Deliberately run job via API in order not to wait for completion.
    When I submit dataproc job
    """
    {
        "name": "sleep",
        "hiveJob": {
            "queryList": {
                "queries": ["select reflect(\"java.lang.Thread\", \"sleep\", bigint(600000));"]
            }
        }
    }
    """
    Then response should have status 200

    # Run some cluster modification operation while job is running.
    # Deliberately modify cluster via API in order to control running time.
    When we attempt to modify cluster "cli_test_cluster" with following parameters
    """
    {
      "labels": {
          "owner": "{{ test_config.cloud_resources_owner }}",
          "foo1": "bar1"
      }
    }
    """
    Then response should have status 200
    And generated task is finished within "4 minutes"


  Scenario: Unable to modify cluster if it is not ALIVE
    Given cluster "cli_test_cluster" is up and running
    When we execute "sudo service hive-server2 stop" on master node of Data Proc cluster
    Then we wait no more than "1 minute" until cluster health is DEGRADED
    When we run YC CLI
    """
    yc dataproc subcluster update data
    --disk-size 24
    --cluster-name cli_test_cluster
    """
    Then YC CLI exit code is 1
    And YC CLI error contains
    """
    Unable to modify cluster while its health status is Degraded: some services are not Alive: Hive is Dead (service is not available)
    """
    When we execute "sudo service hive-server2 start" on master node of Data Proc cluster
    Then we wait no more than "1 minute" until cluster health is ALIVE


  @delete
  Scenario: Delete cluster
    When we run YC CLI
    """
    yc dataproc cluster delete cli_test_cluster
    """
    Then YC CLI exit code is 0
