Feature: Run and cancel jobs on Dataproc cluster

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
    --name cancel_jobs_cluster
    --version "2.1"
    --ssh-public-keys-file staging/mdb_infratest_public_keys
    --services hdfs,yarn,mapreduce,spark
    --bucket {{ outputBucket1 }}
    --subcluster name=leader,role=MASTERNODE,resource-preset=s2.micro,disk-size=20,hosts-count=1,subnet-id={{ defaultSubnetId }}
    --subcluster name=executors,role=DATANODE,resource-preset=s2.micro,disk-size=20,hosts-count=2,subnet-id={{ defaultSubnetId }}
    --service-account-id {{ serviceAccountId1 }}
    --zone {{cluster_config['zoneId']}}
    --log-group-id {{ logGroupId }}
    --labels owner={{ conf['user'] }}
    --property dataproc:version={{ agent_version }}
    --property dataproc:repo={{ agent_repo }}
    """
    Then YC CLI exit code is 0
    And cluster "cancel_jobs_cluster" is up and running
    And YC CLI response contains
    """
    name: cancel_jobs_cluster
    config:
      version_id: "2.1"
      hadoop:
        services:
          - HDFS
          - MAPREDUCE
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
    service_account_id: {{ serviceAccountId1 }}
    """
    And cluster "cancel_jobs_cluster" is up and running


  @immediately @spark
  Scenario: spark job submitted and cancelled immediately
    Given cluster "cancel_jobs_cluster" is up and running
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
    And dataproc job submitted via YC CLI
    When we run YC CLI
    """
    yc dataproc job cancel {{ job_id }}
    """
    Then YC CLI exit code is 0
    Then YC CLI response contains
    """
    {
        "description": "Create Data Proc job"
    }
    """
    Then we wait until dataproc job is "CANCELLED"


  @provisioning @spark
  Scenario: spark job submitted and cancelled while still provisioning
    Given cluster "cancel_jobs_cluster" is up and running
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
    And dataproc job submitted via YC CLI
    Then we wait until dataproc job is "PROVISIONING"
    When we run YC CLI
    """
    yc dataproc job cancel {{ job_id }}
    """
    Then YC CLI exit code is 0
    Then YC CLI response contains
    """
    {
        "description": "Create Data Proc job"
    }
    """
    Then we wait until dataproc job is "CANCELLED"


  @pending @spark
  Scenario: spark job submitted and cancelled after getting accepted by YARN
    Given cluster "cancel_jobs_cluster" is up and running
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
    And we wait until dataproc job is "PENDING,RUNNING"
    When we run YC CLI
    """
    yc dataproc job cancel {{ job_id }}
    """
    Then YC CLI exit code is 0
    Then YC CLI response contains
    """
    {
        "description": "Create Data Proc job"
    }
    """
    Then we wait until dataproc job is "CANCELLED"


  @running @spark
  Scenario: spark job started and cancelled
    Given cluster "cancel_jobs_cluster" is up and running
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
    And we wait until dataproc job is "RUNNING"
    When we run YC CLI
    """
    yc dataproc job cancel {{ job_id }}
    """
    Then YC CLI exit code is 0
    Then YC CLI response contains
    """
    {
        "description": "Create Data Proc job"
    }
    """
    Then we wait until dataproc job is "CANCELLED"


  @immediately @mapreduce
  Scenario: mapreduce job started and cancelled immediately
    Given cluster "cancel_jobs_cluster" is up and running
    And put random string to context under output_folder key

    When we run YC CLI
    """
    yc dataproc job create-mapreduce
    --name "find total urban population on earth"
    --main-class org.apache.hadoop.streaming.HadoopStreaming
    --file-uris s3a://dataproc-e2e/jobs/sources/mapreduce-001/mapper.py
    --file-uris s3a://dataproc-e2e/jobs/sources/mapreduce-001/reducer.py
    --args -mapper --args mapper.py
    --args -reducer --args reducer.py
    --args -numReduceTasks --args 1
    --args -input --args s3a://dataproc-e2e/jobs/sources/data/cities500.txt.bz2
    --args -output --args s3a://{{ outputBucket1 }}/dataproc/clusters/{{ cid }}/jobs/{{ output_folder }}/results
    --async
    """
    Then YC CLI exit code is 0
    And dataproc job submitted via YC CLI
    When we run YC CLI
    """
    yc dataproc job cancel {{ job_id }}
    """
    Then YC CLI exit code is 0
    Then YC CLI response contains
    """
    {
        "description": "Create Data Proc job"
    }
    """
    Then we wait until dataproc job is "CANCELLED"


  @provisioning @mapreduce
  Scenario: mapreduce job submitted and cancelled while still provisioning
    Given cluster "cancel_jobs_cluster" is up and running
    And put random string to context under output_folder key

    When we run YC CLI
    """
    yc dataproc job create-mapreduce
    --name "find total urban population on earth"
    --main-class org.apache.hadoop.streaming.HadoopStreaming
    --file-uris s3a://dataproc-e2e/jobs/sources/mapreduce-001/mapper.py
    --file-uris s3a://dataproc-e2e/jobs/sources/mapreduce-001/reducer.py
    --args -mapper --args mapper.py
    --args -reducer --args reducer.py
    --args -numReduceTasks --args 1
    --args -input --args s3a://dataproc-e2e/jobs/sources/data/cities500.txt.bz2
    --args -output --args s3a://{{ outputBucket1 }}/dataproc/clusters/{{ cid }}/jobs/{{ output_folder }}/results
    --async
    """
    Then YC CLI exit code is 0
    And dataproc job submitted via YC CLI
    Then we wait until dataproc job is "PROVISIONING"
    When we run YC CLI
    """
    yc dataproc job cancel {{ job_id }}
    """
    Then YC CLI exit code is 0
    Then YC CLI response contains
    """
    {
        "description": "Create Data Proc job"
    }
    """
    Then we wait until dataproc job is "CANCELLED"


  @pending @mapreduce
  Scenario: mapreduce job started and cancelled after getting accepted by YARN
    Given cluster "cancel_jobs_cluster" is up and running
    And put random string to context under output_folder key

    When we run YC CLI
    """
    yc dataproc job create-mapreduce
    --name "find total urban population on earth"
    --main-class org.apache.hadoop.streaming.HadoopStreaming
    --file-uris s3a://dataproc-e2e/jobs/sources/mapreduce-001/mapper.py
    --file-uris s3a://dataproc-e2e/jobs/sources/mapreduce-001/reducer.py
    --args -mapper --args mapper.py
    --args -reducer --args reducer.py
    --args -numReduceTasks --args 1
    --args -input --args s3a://dataproc-e2e/jobs/sources/data/cities500.txt.bz2
    --args -output --args s3a://{{ outputBucket1 }}/dataproc/clusters/{{ cid }}/jobs/{{ output_folder }}/results
    --async
    """
    Then YC CLI exit code is 0
    And we wait until dataproc job is "PENDING,RUNNING"
    When we run YC CLI
    """
    yc dataproc job cancel {{ job_id }}
    """
    Then YC CLI exit code is 0
    Then YC CLI response contains
    """
    {
        "description": "Create Data Proc job"
    }
    """
    Then we wait until dataproc job is "CANCELLED"


  @running @mapreduce
  Scenario: mapreduce job started and cancelled
    Given cluster "cancel_jobs_cluster" is up and running
    And put random string to context under output_folder key

    When we run YC CLI
    """
    yc dataproc job create-mapreduce
    --name "find total urban population on earth"
    --main-class org.apache.hadoop.streaming.HadoopStreaming
    --file-uris s3a://dataproc-e2e/jobs/sources/mapreduce-001/mapper.py
    --file-uris s3a://dataproc-e2e/jobs/sources/mapreduce-001/reducer.py
    --args -mapper --args mapper.py
    --args -reducer --args reducer.py
    --args -numReduceTasks --args 1
    --args -input --args s3a://dataproc-e2e/jobs/sources/data/cities500.txt.bz2
    --args -output --args s3a://{{ outputBucket1 }}/dataproc/clusters/{{ cid }}/jobs/{{ output_folder }}/results
    --async
    """
    Then YC CLI exit code is 0
    And we wait until dataproc job is "RUNNING"
    When we run YC CLI
    """
    yc dataproc job cancel {{ job_id }}
    """
    Then YC CLI exit code is 0
    Then YC CLI response contains
    """
    {
        "description": "Create Data Proc job"
    }
    """
    Then we wait until dataproc job is "CANCELLED"


  @immediately @pyspark
  Scenario: pyspark job submitted and cancelled immediately
    Given cluster "cancel_jobs_cluster" is up and running
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
    --jar-file-uris s3a://dataproc-e2e/jobs/sources/java/commons-lang-2.6.jar
    --packages "com.ibm.icu:icu4j:61.1"
    --async
    """
    Then YC CLI exit code is 0
    And dataproc job submitted via YC CLI
    When we run YC CLI
    """
    yc dataproc job cancel {{ job_id }}
    """
    Then YC CLI exit code is 0
    Then YC CLI response contains
    """
    {
        "description": "Create Data Proc job"
    }
    """
    Then we wait until dataproc job is "CANCELLED"


  @provisioning @pyspark
  Scenario: pyspark job submitted and cancelled while still provisioning
    Given cluster "cancel_jobs_cluster" is up and running
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
    --jar-file-uris s3a://dataproc-e2e/jobs/sources/java/commons-lang-2.6.jar
    --packages "com.ibm.icu:icu4j:61.1"
    --async
    """
    Then YC CLI exit code is 0
    And dataproc job submitted via YC CLI
    Then we wait until dataproc job is "PROVISIONING"
    When we run YC CLI
    """
    yc dataproc job cancel {{ job_id }}
    """
    Then YC CLI exit code is 0
    Then YC CLI response contains
    """
    {
        "description": "Create Data Proc job"
    }
    """
    Then we wait until dataproc job is "CANCELLED"


  @delete
  Scenario: Dataproc cluster deleted successfully
    When we run YC CLI
    """
    yc dataproc cluster delete cancel_jobs_cluster
    """
    Then YC CLI exit code is 0
