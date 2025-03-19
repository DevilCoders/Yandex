Feature: Run jobs on Dataproc cluster

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
    --name jobs_cluster
    --ssh-public-keys-file staging/mdb_infratest_public_keys
    --services hdfs,yarn,mapreduce,tez,hive,spark
    --subcluster name=master,role=MASTERNODE,resource-preset=s2.micro,disk-size=20,hosts-count=1,subnet-id={{ defaultSubnetId }}
    --subcluster name=data,role=DATANODE,resource-preset=s2.micro,disk-size=20,hosts-count=2,subnet-id={{ defaultSubnetId }}
    --service-account-id {{ serviceAccountId1 }}
    --zone {{cluster_config['zoneId']}}
    --log-group-id {{ logGroupId }}
    --labels owner={{ conf['user'] }}
    --property yarn:yarn.nm.liveness-monitor.expiry-interval-ms=15000
    --property yarn:yarn.log-aggregation-enable=false
    --property dataproc:version={{ agent_version }}
    --property dataproc:repo={{ agent_repo }}
    """
    Then YC CLI exit code is 0
    And cluster "jobs_cluster" is up and running
    And YC CLI response contains
    """
    name: jobs_cluster
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
    """
    And cluster "jobs_cluster" is up and running

  @run @pyspark @cli
  Scenario: Pyspark job may be submitted
    Given cluster "jobs_cluster" is up and running

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
            "s3a://dataproc-e2e/jobs/sources/java/commons-lang-2.6.jar"
          ],
          "file_uris": [
            "s3a://dataproc-e2e/jobs/sources/data/config.json"
          ],
          "archive_uris": [
            "s3a://dataproc-e2e/jobs/sources/data/country-codes.csv.zip"
          ],
          "main_python_file_uri": "s3a://dataproc-e2e/jobs/sources/pyspark-001/main.py",
          "packages": [
            "com.ibm.icu:icu4j:61.1"
          ],
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

    When we run YC CLI
    """
    yc dataproc job get {{ job_id }}
    """
    Then YC CLI exit code is 0
    Then YC CLI response contains
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
            "s3a://dataproc-e2e/jobs/sources/java/commons-lang-2.6.jar"
          ],
          "file_uris": [
            "s3a://dataproc-e2e/jobs/sources/data/config.json"
          ],
          "archive_uris": [
            "s3a://dataproc-e2e/jobs/sources/data/country-codes.csv.zip"
          ],
          "main_python_file_uri": "s3a://dataproc-e2e/jobs/sources/pyspark-001/main.py",
          "packages": [
            "com.ibm.icu:icu4j:61.1"
          ],
          "python_file_uris": [
            "s3a://dataproc-e2e/jobs/sources/pyspark-001/geonames.py"
          ]
        }
    }
    """

  @run @pyspark @cli
  Scenario: YC CLI handles job errors correctly
    Given cluster "jobs_cluster" is up and running

    When we run YC CLI
    """
    yc dataproc job create-pyspark
    --name "job with error"
    --main-python-file-uri s3a://dataproc-e2e/jobs/sources/pyspark-001/main.py
    --python-file-uris s3a://dataproc-e2e/jobs/sources/pyspark-001/geonames.py
    --properties spark.yarn.queue=missing
    """
    Then YC CLI exit code is 1
    Then dataproc job started via YC CLI is ERROR
    And job driver output contains
    """
    submitted by user dataproc-agent to unknown queue: missing
    """


  @run @hive @cli
  Scenario: Hive job allows to execute sql script file and pass variables to it
    Given cluster "jobs_cluster" is up and running

    When we run YC CLI
    """
    yc dataproc job create-hive
    --name "find total population of cities in Russia"
    --query-file-uri s3a://dataproc-e2e/jobs/sources/hive-001/main.sql
    --script-variables CITIES_URI=s3a://dataproc-e2e/jobs/sources/data/hive-cities/,COUNTRY_CODE=RU
    """
    Then YC CLI exit code is 0
    And dataproc job started via YC CLI is DONE
    And YC CLI response contains
    """
    {
        "name": "find total population of cities in Russia",
        "status": "DONE",
        "hive_job": {
            "script_variables": {
                "CITIES_URI": "s3a://dataproc-e2e/jobs/sources/data/hive-cities/",
                "COUNTRY_CODE": "RU"
            },
            "query_file_uri": "s3a://dataproc-e2e/jobs/sources/hive-001/main.sql"
        }
    }
    """
    And job driver output contains
    """
    128173232
    """


  @run @hive @cli
  Scenario: Hive job allows to execute inline sql queries and set hive properties
    Given cluster "jobs_cluster" is up and running

    When we run YC CLI
    """
    yc dataproc job create-hive
    --name "find number of cities in Russia and their population"
    --query-list "SELECT count(*) as cities_count FROM cities WHERE country_code='RU';"
    --query-list "SELECT sum(population) as total_population FROM cities WHERE country_code='RU';"
    --properties hive.cli.print.header=true
    """
    Then YC CLI exit code is 0
    And dataproc job started via YC CLI is DONE
    And YC CLI response contains
    """
    {
        "name": "find number of cities in Russia and their population",
        "status": "DONE",
        "hive_job": {
            "properties": {
                "hive.cli.print.header": "true"
            },
            "query_list": {
                "queries": [
                    "SELECT count(*) as cities_count FROM cities WHERE country_code='RU';",
                    "SELECT sum(population) as total_population FROM cities WHERE country_code='RU';"
                ]
            }
        }
    }
    """
    And job driver output contains
    """
    cities_count
    """
    And job driver output contains
    """
    5003
    """
    And job driver output contains
    """
    total_population
    """
    And job driver output contains
    """
    128173232
    """


  @run @hive
  Scenario: Hive job fails on first error by default
    Given cluster "jobs_cluster" is up and running

    When we run dataproc job
    """
    {
        "name": "find number of countries",
        "hiveJob": {
            "queryList": {
                "queries": [
                    "SELECT count(*) FROM countries;",
                    "SELECT count(*) FROM cities WHERE country_code='RU';"
                ]
            }
        }
    }
    """
    Then response should have status 200
    And created job fails within "5 minutes"
    And job driver output contains
    """
    Table not found 'countries'
    """
    And job driver output does not contain
    """
    5003
    """


  @run @hive
  Scenario: Hive job continues execution after error if continueOnFailure is set to true
    Given cluster "jobs_cluster" is up and running

    When we run dataproc job
    """
    {
        "name": "find number of countries",
        "hiveJob": {
            "continueOnFailure": true,
            "queryList": {
                "queries": [
                    "SELECT count(*) FROM countries;",
                    "SELECT count(*) FROM cities WHERE country_code='RU';"
                ]
            }
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And job driver output contains
    """
    Table not found 'countries'
    """
    And job driver output contains
    """
    5003
    """


  @run @hive
  Scenario: Hive job do not fail if last query fails when continueOnFailure is set to true
    Given cluster "jobs_cluster" is up and running

    When we run dataproc job
    """
    {
        "name": "find number of countries",
        "hiveJob": {
            "continueOnFailure": true,
            "queryList": {
                "queries": [
                    "SELECT count(*) FROM cities WHERE country_code='RU';",
                    "SELECT count(*) FROM countries;"
                ]
            }
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And job driver output contains
    """
    5003
    """
    And job driver output contains
    """
    Table not found 'countries'
    """


  @run @hive
  Scenario: Hive job allows to load and use remote jar within sql script
    Given cluster "jobs_cluster" is up and running

    When we run dataproc job
    """
    {
        "name": "find biggest city within 200 km from Moscow",
        "hiveJob": {
            "queryFileUri": "s3a://dataproc-e2e/jobs/sources/hive-001/geodistance-2.sql"
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And job driver output contains
    """
    Ryazan’  | 520173
    """


  @run @hive
  Scenario: Hive job allows to use local jar file
    Given cluster "jobs_cluster" is up and running

    When we run dataproc job
    """
    {
        "name": "copy geodistance jar from s3 to master node",
        "hiveJob": {
            "queryList": {
                "queries": ["dfs -cp -f s3a://dataproc-e2e/jobs/sources/java/dataproc-examples-1.0.jar file:///tmp/hive/dataproc-examples-1.0.jar;"]
            }
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    When we run dataproc job
    """
    {
        "name": "find biggest city within 200 km from Moscow",
        "hiveJob": {
            "jarFileUris": ["/tmp/hive/dataproc-examples-1.0.jar"],
            "queryList": {
                "queries": [
                    "CREATE TEMPORARY FUNCTION geodistance AS 'ru.yandex.cloud.dataproc.examples.GeoDistance';",
                    "SELECT name, population FROM cities WHERE geodistance(55.75222, 37.61556, latitude, longitude) < 200 ORDER BY population DESC LIMIT 2, 1;"
                ]
            }
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"
    And job driver output contains
    """
    Ryazan’  | 520173
    """


  @run @mapreduce @cli
  Scenario: Hadoop streaming allows to write map/reduce tasks using python
    Given cluster "jobs_cluster" is up and running
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
    --properties yarn.app.mapreduce.am.resource.mb=2048
    --properties yarn.app.mapreduce.am.command-opts=-Xmx2048m
    --properties mapreduce.job.maps=6
    """
    Then YC CLI exit code is 0
    And dataproc job started via YC CLI is DONE
    And s3 file s3://{{ outputBucket1 }}/dataproc/clusters/{{ cid }}/jobs/{{ output_folder }}/results/part-00000 contains
    """
    3157665119
    """


  @run @mapreduce @cli
  Scenario: Dataproc allows to run mapreduce jobs written in java
    Given cluster "jobs_cluster" is up and running
    And put random string to context under output_folder key

    When we run dataproc job
    """
    {
        "name": "copy commons-lang-2.6.jar from s3 to hdfs",
        "hiveJob": {
            "queryList": {
                "queries": ["dfs -cp -f s3a://dataproc-e2e/jobs/sources/java/commons-lang-2.6.jar hdfs:///var/commons-lang-2.6.jar;"]
            }
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "5 minutes"

    When we run YC CLI
    """
    yc dataproc job create-mapreduce
    --name "find total urban population within Russia"
    --main-jar-file-uri s3a://dataproc-e2e/jobs/sources/java/dataproc-examples-1.0.jar
    --jar-file-uris https://repo1.maven.org/maven2/com/opencsv/opencsv/4.1/opencsv-4.1.jar
    --jar-file-uris s3a://dataproc-e2e/jobs/sources/java/icu4j-61.1.jar
    --jar-file-uris hdfs:///var/commons-lang-2.6.jar
    --jar-file-uris s3a://dataproc-e2e/jobs/sources/java/json-20190722.jar
    --file-uris s3a://dataproc-e2e/jobs/sources/data/config.json
    --archive-uris s3a://dataproc-e2e/jobs/sources/data/country-codes.csv.zip
    --args ru.yandex.cloud.dataproc.examples.PopulationMRJob
    --args s3a://dataproc-e2e/jobs/sources/data/cities500.txt.bz2
    --args s3a://{{ outputBucket1 }}/dataproc/clusters/{{ cid }}/jobs/{{ output_folder }}/results
    --properties yarn.app.mapreduce.am.resource.mb=2048
    --properties yarn.app.mapreduce.am.command-opts=-Xmx2048m
    --properties mapreduce.map.memory.mb=2048
    --properties mapreduce.map.java.opts=-Xmx2048m
    --properties mapreduce.reduce.memory.mb=2048
    --properties mapreduce.reduce.java.opts=-Xmx2048m
    --properties mapreduce.input.fileinputformat.split.minsize=1300000
    --properties mapreduce.input.fileinputformat.split.maxsize=1500000
    """
    Then YC CLI exit code is 0
    And dataproc job started via YC CLI is DONE
    And combined content of files within s3 folder s3://{{ outputBucket1 }}/dataproc/clusters/{{ cid }}/jobs/{{ output_folder }}/results contains
    """
    Rossiyskaya Federatsiya	128173232
    """


  @run @mapreduce
  Scenario: Mapreduce job's Args should not contain main class name if it may be extracted from manifest of main jar
    Given cluster "jobs_cluster" is up and running
    And put random string to context under output_folder key

    When we run dataproc job
    """
    {
        "name": "find total urban population within Russia",
        "mapreduceJob": {
            "mainJarFileUri": "s3a://dataproc-e2e/jobs/sources/java/dataproc-examples-1.0-with-main-class.jar",
            "jarFileUris": [
                "s3a://dataproc-e2e/jobs/sources/java/opencsv-4.1.jar",
                "s3a://dataproc-e2e/jobs/sources/java/icu4j-61.1.jar",
                "s3a://dataproc-e2e/jobs/sources/java/commons-lang-2.6.jar",
                "s3a://dataproc-e2e/jobs/sources/java/json-20190722.jar"
            ],
            "fileUris": [
                "s3a://dataproc-e2e/jobs/sources/data/config.json"
            ],
            "archiveUris": [
                "s3a://dataproc-e2e/jobs/sources/data/country-codes.csv.zip"
            ],
            "args": [
                "s3a://dataproc-e2e/jobs/sources/data/cities500.txt.bz2",
                "s3a://{{ outputBucket1 }}/dataproc/clusters/{{ cid }}/jobs/{{ output_folder }}/results"
            ],
            "properties": {
                "yarn.app.mapreduce.am.resource.mb": "2048",
                "yarn.app.mapreduce.am.command-opts": "-Xmx2048m",
                "mapreduce.map.memory.mb": "2048",
                "mapreduce.map.java.opts": "-Xmx2048m",
                "mapreduce.reduce.memory.mb": "2048",
                "mapreduce.reduce.java.opts": "-Xmx2048m",
                "mapreduce.input.fileinputformat.split.minsize": "1300000",
                "mapreduce.input.fileinputformat.split.maxsize": "1500000"
            }
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "7 minutes"
    And combined content of files within s3 folder s3://{{ outputBucket1 }}/dataproc/clusters/{{ cid }}/jobs/{{ output_folder }}/results contains
    """
    Rossiyskaya Federatsiya	128173232
    """


  @run @mapreduce @cli
  Scenario: Mapreduce job allows to run hadoop-mapreduce-examples
    Given cluster "jobs_cluster" is up and running

    When we run YC CLI
    """
    yc dataproc job create-mapreduce
    --name "calculate PI"
    --main-jar-file-uri /usr/lib/hadoop-mapreduce/hadoop-mapreduce-examples.jar
    --args pi --args 1 --args 1000
    """
    Then YC CLI exit code is 0
    And dataproc job started via YC CLI is DONE
    And job driver output contains
    """
    Estimated value of Pi is
    """


  @run @mapreduce
  Scenario: Mapreduce job reports error correctly when jar file is missing
    Given cluster "jobs_cluster" is up and running
    And put random string to context under output_folder key

    When we run dataproc job
    """
    {
        "name": "mapreduce job with error",
        "mapreduceJob": {
            "mainJarFileUri": "s3a://dataproc-e2e/jobs/sources/java/missing-file.jar"
        }
    }
    """
    Then response should have status 200
    And created job fails within "5 minutes"
    And job driver output contains
    """
    `s3a://dataproc-e2e/jobs/sources/java/missing-file.jar': No such file or directory
    """


  @run @mapreduce
  Scenario: Mapreduce job reports error correctly when jar http file is missing
    Given cluster "jobs_cluster" is up and running
    And put random string to context under output_folder key

    When we run dataproc job
    """
    {
        "name": "mapreduce job with error",
        "mapreduceJob": {
            "mainJarFileUri": "http://example.com/missing-file.jar"
        }
    }
    """
    Then response should have status 200
    And created job fails within "5 minutes"
    And job driver output contains
    """
    failed to download file http://example.com/missing-file.jar: 404 Not Found
    """


  @run @mapreduce
  Scenario: Mapreduce job reports error correctly when invalid jar file provided
    Given cluster "jobs_cluster" is up and running
    And put random string to context under output_folder key

    When we run dataproc job
    """
    {
        "name": "mapreduce job with error",
        "mapreduceJob": {
            "mainJarFileUri": "s3a://dataproc-e2e/jobs/sources/java/invalid-jar.jar"
        }
    }
    """
    Then response should have status 200
    And created job fails within "5 minutes"
    And job driver output contains
    """
    failed to open job JAR s3a://dataproc-e2e/jobs/sources/java/invalid-jar.jar: zip: not a valid zip file
    """


  @run @mapreduce
  Scenario: Mapreduce job reports error correctly when path to jar file has unsupported protocol
    Given cluster "jobs_cluster" is up and running
    And put random string to context under output_folder key

    When we run dataproc job
    """
    {
        "name": "mapreduce job with error",
        "mapreduceJob": {
            "mainJarFileUri": "xxx://path/job.jar"
        }
    }
    """
    Then response should have status 200
    And created job fails within "5 minutes"
    And job driver output contains
    """
    failed to download file xxx://path/job.jar: No FileSystem for scheme "xxx"
    """


  @run @mapreduce
  Scenario: Mapreduce job reports error correctly when invalid bucket specified for jar file
    Given cluster "jobs_cluster" is up and running
    And put random string to context under output_folder key

    When we run dataproc job
    """
    {
        "name": "mapreduce job with error",
        "mapreduceJob": {
            "mainJarFileUri": "s3a://missing-bucket-name/job.jar"
        }
    }
    """
    Then response should have status 200
    And created job fails within "5 minutes"
    And job driver output contains
    """
    failed to download file s3a://missing-bucket-name/job.jar: Bucket missing-bucket-name does not exist
    """

  @run @spark @cli
  Scenario: Spark job executed successfully
    Given cluster "jobs_cluster" is up and running

    When we run YC CLI
    """
    yc dataproc job create-spark
    --name "find total urban population in distribution by country"
    --main-jar-file-uri s3a://dataproc-e2e/jobs/sources/java/dataproc-examples-1.0.jar
    --main-class ru.yandex.cloud.dataproc.examples.PopulationSparkJob
    --args s3a://dataproc-e2e/jobs/sources/data/cities500.txt.bz2
    --args s3a://{{ outputBucket1 }}/dataproc/clusters/{{ cid }}/jobs/\${JOB_ID}/results
    --file-uris s3a://dataproc-e2e/jobs/sources/data/config.json
    --jar-file-uris s3a://dataproc-e2e/jobs/sources/java/json-20190722.jar
    --archive-uris s3a://dataproc-e2e/jobs/sources/data/country-codes.csv.zip
    --packages "com.opencsv:opencsv:4.1"
    --packages "com.ibm.icu:icu4j:61.1"
    """
    Then YC CLI exit code is 0
    And dataproc job started via YC CLI is DONE
    And s3 file s3://{{ outputBucket1 }}/dataproc/clusters/{{ cid }}/jobs/{{ job_id }}/results/part-00000 contains
    """
    Rossiyskaya Federatsiya,128173232
    """
    And job driver output contains
    """
    final status: SUCCEEDED
    """

  @cancel @pending @pyspark
  Scenario: pyspark job submitted and cancelled after getting accepted by YARN
    Given cluster "jobs_cluster" is up and running
    When we run YC CLI
    """
    yc dataproc job create-pyspark
    --name "find total population of cities in Russia"
    --main-python-file-uri "s3a://dataproc-e2e/jobs/sources/pyspark-001/main_with_sleep.py"
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

  @cancel @running @pyspark
  Scenario: pyspark job started and cancelled
    Given cluster "jobs_cluster" is up and running
    When we run YC CLI
    """
    yc dataproc job create-pyspark
    --name "find total population of cities in Russia"
    --main-python-file-uri "s3a://dataproc-e2e/jobs/sources/pyspark-001/main_with_sleep.py"
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


  @run
  Scenario: Unable to run job if HDFS is in safemode
    Given cluster "jobs_cluster" is up and running
    When we execute "sudo -u hdfs hdfs dfsadmin -safemode enter" on master node
    Then we wait no more than "1 minute" until dataproc manager returns cluster health response containing
    """
    "hdfsInSafemode": true
    """
    When we run YC CLI
    """
    yc dataproc job create-hive --query-list "SELECT count(*) FROM cities;"
    """
    Then YC CLI exit code is 1
    And YC CLI error contains
    """
    Unable to run jobs while HDFS is in Safemode
    """
    When we execute "sudo -u hdfs hdfs dfsadmin -safemode leave" on master node
    Then we wait no more than "1 minute" until dataproc manager returns cluster health response containing
    """
    "hdfsInSafemode": false
    """


  @run @cli
  Scenario: YC CLI allows to list jobs
    Given cluster "jobs_cluster" is up and running

    When we run YC CLI
    """
    yc dataproc job list
    """
    Then YC CLI exit code is 0


  @delete
  Scenario: Dataproc cluster deleted successfully
    When we run YC CLI
    """
    yc dataproc cluster delete jobs_cluster
    """
    Then YC CLI exit code is 0
