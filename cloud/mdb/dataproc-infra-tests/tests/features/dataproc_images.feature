Feature: Run jobs on Dataproc cluster

    Background: Wait until internal api is ready
        Given Internal API is up and running
        Given we are working with standard Hadoop cluster

    @images @image1.4 @create @cli
    Scenario: Dataproc cluster with image 1.4 with full service list created successfully
        Given we put ssh public keys to staging/mdb_infratest_public_keys
        When we run YC CLI retrying ResourceExhausted exception for "25 minutes"
    """
    yc dataproc cluster create
    --version 1.4
    --name dataproc-1-4
    --ssh-public-keys-file staging/mdb_infratest_public_keys
    --services hdfs,yarn,mapreduce,hive,hbase,tez,spark,flume,oozie,sqoop,zeppelin,zookeeper
    --subcluster name=master,role=MASTERNODE,resource-preset=s2.small,disk-size=20,hosts-count=1,subnet-id={{ defaultSubnetId }}
    --subcluster name=data,role=DATANODE,resource-preset=s2.micro,disk-size=20,hosts-count=1,subnet-id={{ defaultSubnetId }}
    --service-account-id {{ serviceAccountId1 }}
    --bucket {{ outputBucket1 }}
    --zone {{cluster_config['zoneId']}}
    --labels owner={{ conf['user'] }}
    """
        Then YC CLI exit code is 0
        And cluster "dataproc-1-4" is up and running


    @images @image1.4 @delete @cli
    Scenario: Dataproc cluster with image 1.4 deleted successfully
        Given cluster "dataproc-1-4" exists
        When we run YC CLI
    """
    yc dataproc cluster delete dataproc-1-4
    """
        Then YC CLI exit code is 0


  @images @image2.0 @create @cli
  Scenario: Dataproc cluster with image 2.0 with full service list created successfully
    Given we put ssh public keys to staging/mdb_infratest_public_keys
    When we run YC CLI retrying ResourceExhausted exception for "20 minutes"
    """
    yc dataproc cluster create
    --version 2.0
    --name dataproc-2-0
    --ssh-public-keys-file staging/mdb_infratest_public_keys
    --services hdfs,yarn,mapreduce,hive,hbase,tez,spark,zeppelin,zookeeper,oozie,livy
    --subcluster name=master,role=MASTERNODE,resource-preset=s2.small,disk-size=20,hosts-count=1,subnet-id={{ defaultSubnetId }}
    --subcluster name=data,role=DATANODE,resource-preset=s2.micro,disk-size=20,hosts-count=1,subnet-id={{ defaultSubnetId }}
    --service-account-id {{ serviceAccountId1 }}
    --bucket {{ outputBucket1 }}
    --zone {{cluster_config['zoneId']}}
    --labels owner={{ conf['user'] }}
    """
    Then YC CLI exit code is 0
    And cluster "dataproc-2-0" is up and running


  @images @image2.0 @delete @cli
  Scenario: Dataproc cluster with image 2.0 deleted successfully
    Given cluster "dataproc-2-0" exists
    When we run YC CLI
    """
    yc dataproc cluster delete dataproc-2-0
    """
    Then YC CLI exit code is 0
