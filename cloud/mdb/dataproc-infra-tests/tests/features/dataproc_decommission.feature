Feature: Create Compute Dataproc cluster

  Background: Wait until internal api is ready
    Given Internal API is up and running
    Given we are working with standard Hadoop cluster


  @create
  Scenario: Create cluster
    Given we put ssh public keys to staging/mdb_infratest_public_keys
    When we run YC CLI retrying ResourceExhausted exception for "20 minutes"
    """
    yc dataproc cluster create
    --name test_cluster
    --description "Initial description"
    --ssh-public-keys-file staging/mdb_infratest_public_keys
    --services hdfs,yarn,mapreduce,tez,hive,spark
    --subcluster name=master,role=MASTERNODE,resource-preset=s2.small,disk-size=20,hosts-count=1,subnet-id={{ defaultSubnetId }}
    --subcluster name=data,role=DATANODE,resource-preset=s2.micro,disk-size=20,hosts-count=3,subnet-id={{ defaultSubnetId }}
    --service-account-id {{ serviceAccountId1 }}
    --bucket {{ outputBucket1 }}
    --zone {{cluster_config['zoneId']}}
    --log-group-id {{ logGroupId }}
    --labels owner={{ conf['user'] }}
    --property yarn:yarn.nm.liveness-monitor.expiry-interval-ms=15000
    --property hdfs:dfs.blocksize=4m
    --property yarn:yarn.log-aggregation-enable=false
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
          "yarn:yarn.nm.liveness-monitor.expiry-interval-ms": "15000"
          "hdfs:dfs.blocksize": "4m"
          "yarn:yarn.log-aggregation-enable": "false"
    health: ALIVE
    zone_id: {{cluster_config['zoneId']}}
    service_account_id: {{ serviceAccountId1 }}
    bucket: {{ outputBucket1 }}
    """
    And cluster "test_cluster" is up and running
    Then update dataproc agent to the latest version


  @create_file
  Scenario: Place file to hdfs
    Given cluster "test_cluster" is up and running
    When we try to make "64MB" random file "/random_64mb" in hdfs
    Then no node in cluster holds all blocks of "/random_64mb"


  @decommission_subcluster_data_nodes
  Scenario: Data nodes number is decreased, their resources is increased
    Given cluster "test_cluster" is up and running
    And put random string to context under output_folder key

    When we run YC CLI
    """
    yc dataproc job create-mapreduce
    --name "find total urban population within Russia"
    --main-jar-file-uri "s3a://dataproc-e2e/jobs/sources/java/dataproc-examples-1.0-with-main-class.jar"
    --jar-file-uris "s3a://dataproc-e2e/jobs/sources/java/opencsv-4.1.jar"
    --jar-file-uris "s3a://dataproc-e2e/jobs/sources/java/icu4j-61.1.jar"
    --jar-file-uris "s3a://dataproc-e2e/jobs/sources/java/commons-lang-2.6.jar"
    --jar-file-uris "s3a://dataproc-e2e/jobs/sources/java/json-20190722.jar"
    --file-uris "s3a://dataproc-e2e/jobs/sources/data/config.json"
    --archive-uris "s3a://dataproc-e2e/jobs/sources/data/country-codes.csv.zip"
    --args "s3a://dataproc-e2e/jobs/sources/data/cities500.txt.bz2"
    --args "s3a://{{ outputBucket1 }}/dataproc/clusters/{{ cid }}/jobs/{{ output_folder }}/results"
    --properties yarn.app.mapreduce.am.resource.mb=2048
    --properties yarn.app.mapreduce.am.command-opts=-Xmx2048m
    --properties mapreduce.map.memory.mb=2048
    --properties mapreduce.map.java.opts=-Xmx2048m
    --properties mapreduce.reduce.memory.mb=2048
    --properties mapreduce.reduce.java.opts=-Xmx2048m
    --properties mapreduce.input.fileinputformat.split.minsize=1300000
    --properties mapreduce.input.fileinputformat.split.maxsize=1500000
    --async
    """
    Then YC CLI exit code is 0
    And we wait no more than "10 minutes" until dataproc job is "RUNNING"
    When we run YC CLI retrying ResourceExhausted exception for "20 minutes"
    """
    yc dataproc subcluster update data
    --cluster-name test_cluster
    --resource-preset s2.small
    --hosts-count 2
    --decommission-timeout 540
    """
    Then YC CLI exit code is 0
    And YC CLI response contains
    """
    hosts_count: '2'
    """
    And dataproc job after another CLI operation started via YC CLI is DONE

  @pyspark
  Scenario: Pyspark job may be submitted
    Given cluster "test_cluster" is up and running

    When we run YC CLI
    """
    yc dataproc job create-pyspark
    --name "find total population of cities in Russia"
    --main-python-file-uri "s3a://dataproc-e2e/jobs/sources/pyspark-001/main.py"
    --python-file-uris "s3a://dataproc-e2e/jobs/sources/pyspark-001/geonames.py"
    --args s3a://dataproc-e2e/jobs/sources/data/cities500.txt.bz2
    --args s3a://{{ outputBucket1 }}/dataproc/clusters/{{ cid }}/jobs/\${JOB_ID}/results
    --file-uris "s3a://dataproc-e2e/jobs/sources/data/config.json"
    --archive-uris "s3a://dataproc-e2e/jobs/sources/data/country-codes.csv.zip"
    --jar-file-uris s3a://dataproc-e2e/jobs/sources/java/dataproc-examples-1.0.jar
    --jar-file-uris s3a://dataproc-e2e/jobs/sources/java/icu4j-61.1.jar
    --jar-file-uris s3a://dataproc-e2e/jobs/sources/java/commons-lang-2.6.jar
    """
    Then YC CLI exit code is 0
    Then dataproc job started via YC CLI is DONE
    And YC CLI response contains
    """
    {
        "name": "find total population of cities in Russia",
        "status": "DONE",
        "pyspark_job": {
          "args": [
            "s3a://dataproc-e2e/jobs/sources/data/cities500.txt.bz2",
            "s3a://{{ outputBucket1 }}/dataproc/clusters/{{ cid }}/jobs/${JOB_ID}/results"
          ],
          "jar_file_uris": [
            "s3a://dataproc-e2e/jobs/sources/java/dataproc-examples-1.0.jar",
            "s3a://dataproc-e2e/jobs/sources/java/icu4j-61.1.jar",
            "s3a://dataproc-e2e/jobs/sources/java/commons-lang-2.6.jar"
          ],
          "file_uris": [
            "s3a://dataproc-e2e/jobs/sources/data/config.json"
          ],
          "archive_uris": [
            "s3a://dataproc-e2e/jobs/sources/data/country-codes.csv.zip"
          ],
          "main_python_file_uri": "s3a://dataproc-e2e/jobs/sources/pyspark-001/main.py",
          "python_file_uris": [
            "s3a://dataproc-e2e/jobs/sources/pyspark-001/geonames.py"
          ]
        }
    }
    """
    And s3 file s3://{{ outputBucket1 }}/dataproc/clusters/{{ cid }}/jobs/{{ job_id }}/results/part-00000 contains
    """
    Rossiyskaya Federatsiya,128173232
    """
    And job driver output contains
    """
    final status: SUCCEEDED
    """

  @subcluster
  Scenario: Create subcluster
    Given cluster "test_cluster" is up and running

    When we run YC CLI retrying ResourceExhausted exception for "20 minutes"
    """
    yc dataproc subcluster create
    --cluster-name test_cluster
    --name compute
    --role COMPUTENODE
    --resource-preset s2.micro
    --disk-type network-hdd
    --subnet-id {{ defaultSubnetId }}
    --disk-size 20
    --hosts-count 2
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
      disk_size: "21474836480"
    subnet_id: "{{ defaultSubnetId }}"
    hosts_count: "2"
    """


  @decommission_cluster_nodes
  Scenario: Cluster node number is decreased to 1 computenode and 1 datanode
    Given cluster "test_cluster" is up and running
    When we attempt to modify cluster "test_cluster" with following parameters
    """
    {
      "decommissionTimeout": 540,
      "configSpec": {
        "subclustersSpec": [
          {
            "id": "{{subclusters_id_by_name['master']}}",
            "name": "main"
          },
          {
            "id": "{{subclusters_id_by_name['data']}}",
            "hostsCount": 1
          },
          {
            "id": "{{subclusters_id_by_name['compute']}}",
            "hostsCount": 1
          }
        ]
      }
    }
    """
    Then response should have status 200
    And generated task is finished within "10 minutes"
    When we GET subcluster with name "data"
    Then response should have status 200 and body contains
    """
    {
      "hostsCount": 1,
      "name": "data",
      "resources": {
        "diskSize": 21474836480,
        "diskTypeId": "network-hdd",
        "resourcePresetId": "s2.small"
      },
      "role": "DATANODE"
    }
    """
    When we GET subcluster with name "compute"
    Then response should have status 200 and body contains
    """
    {
      "hostsCount": 1,
      "name": "compute",
      "resources": {
        "diskSize": 21474836480,
        "diskTypeId": "network-hdd",
        "resourcePresetId": "s2.micro"
      },
      "role": "COMPUTENODE"
    }
    """


  @check_nodes
  Scenario: Check file datanodes number
    Given cluster "test_cluster" is up and running
    Then there is a node that contains all blocks of "/random_64mb"
    And hash sum of "/random_64mb" from hdfs is ok


  @delete
  Scenario: Dataproc cluster deleted successfully
    When we run YC CLI
    """
    yc dataproc cluster delete test_cluster --decommission-timeout 300
    """
    Then YC CLI exit code is 0
