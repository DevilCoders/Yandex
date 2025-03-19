Feature: Dataproc cluster on dedicated hosts

  Background: Wait until internal api is ready
    Given Internal API is up and running
    Given we are working with standard Hadoop cluster


  @create
  Scenario: Create dataproc cluster on dedicated host
    Given we put ssh public keys to staging/mdb_infratest_public_keys
    When we run YC CLI retrying ResourceExhausted exception for "20 minutes"
    """
    yc dataproc cluster create
    --name host_group_cluster
    --ssh-public-keys-file staging/mdb_infratest_public_keys
    --services hdfs,yarn,mapreduce,tez,hive,spark
    --subcluster name=master,role=MASTERNODE,resource-preset=s2.small,disk-size=200,hosts-count=1,subnet-id={{ subnetWithHostGroup }}
    --subcluster name=data,role=DATANODE,resource-preset=s2.micro,disk-size=200,disk-type=network-ssd,hosts-count=2,subnet-id={{ subnetWithHostGroup }}
    --subcluster name=compute-autoscaling,role=COMPUTENODE,preemptible=true,resource-preset=s2.micro,disk-size=100,hosts-count=1,max-hosts-count=4,subnet-id={{ subnetWithHostGroup }},stabilization-duration=2m
    --service-account-id {{ serviceAccountId2 }}
    --bucket {{ outputBucket1 }}
    --zone {{ zoneWithHostGroup }}
    --labels owner={{ conf['user'] }}
    --log-group-id {{ logGroupId }}
    --host-group-ids {{ hostGroupId }}
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    host_group_ids:
    - {{ hostGroupId }}
    """
    And cluster "host_group_cluster" is up and running


  Scenario: Hosts added via update cluster request are placed onto dedicated host
    Given cluster "host_group_cluster" is up and running
    # intentionally use api instead of cli, because we want to modify cluster, not subcluster
    When we attempt to modify cluster "host_group_cluster" with following parameters
    """
    {
      "configSpec": {
        "subclustersSpec": [
          {
            "id": "{{subclusters_id_by_name['data']}}",
            "hostsCount": 2
          }
        ]
      }
    }
    """
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And all hosts of current cluster reside on the same dedicated host


  Scenario: Hosts of new subcluster are placed onto dedicated host
    Given cluster "host_group_cluster" is up and running

    When we run YC CLI retrying ResourceExhausted exception for "20 minutes"
    """
    yc dataproc subcluster create
    --cluster-name host_group_cluster
    --name compute
    --role COMPUTENODE
    --resource-preset s2.micro
    --disk-type network-hdd
    --subnet-id {{ subnetWithHostGroup }}
    --disk-size 20
    --hosts-count 1
    """
    Then YC CLI exit code is 0
    And all hosts of current cluster reside on the same dedicated host


  Scenario: Hosts added via update subcluster request are placed onto dedicated host
    Given cluster "host_group_cluster" is up and running

    When we run YC CLI retrying ResourceExhausted exception for "20 minutes"
    """
    yc dataproc subcluster update compute
    --hosts-count 2
    --cluster-name host_group_cluster
    """
    Then YC CLI exit code is 0
    And all hosts of current cluster reside on the same dedicated host


  @delete
  Scenario: Delete cluster
    When we run YC CLI
    """
    yc dataproc cluster delete host_group_cluster
    """
    Then YC CLI exit code is 0
