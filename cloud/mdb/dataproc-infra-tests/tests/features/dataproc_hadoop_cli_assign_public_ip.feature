Feature: Create Dataproc cluster with public IP using CLI

  Background: Wait until internal api is ready
    Given Internal API is up and running
    Given we are working with standard Hadoop cluster
    Given username ubuntu


  @create
  Scenario: Create cluster with public IP on master and data subclusters and with 1 security-group
    Given we put ssh public keys to staging/mdb_infratest_public_keys
    When we run YC CLI retrying ResourceExhausted exception for "20 minutes"
    """
    yc dataproc cluster create
    --name cli_test_cluster
    --description "Initial description"
    --ssh-public-keys-file staging/mdb_infratest_public_keys
    --services hdfs,yarn,mapreduce,tez,hive,spark
    --subcluster name=master,role=MASTERNODE,resource-preset=s2.small,disk-size=20,hosts-count=1,assign-public-ip=True,subnet-id={{ defaultSubnetId }}
    --subcluster name=data,role=DATANODE,resource-preset=s2.micro,disk-size=20,hosts-count=1,assign-public-ip=True,subnet-id={{ defaultSubnetId }}
    --service-account-id {{ serviceAccountId1 }}
    --security-group-ids c6452e60lj7i7iuklajq
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


  @delete
  Scenario: Delete cluster
    When we run YC CLI
    """
    yc dataproc cluster delete cli_test_cluster
    """
    Then YC CLI exit code is 0
