Feature: Dataproc cluster run benchmark

  Background: Wait until internal api is ready
    Given Internal API is up and running
    Given we are working with benchmark Hadoop cluster


  @create @hdfs
  Scenario: Dataproc cluster created successfully
    When we try to create cluster "benchmark_hdfs_io_cluster"
    Then response should have status 200
    And generated task is finished within "10 minutes"


  @testdfsio_benchmark @write @hdfs
  Scenario: Benchmark write to HDFS
    Given cluster "benchmark_hdfs_io_cluster" is up and running
    When we run dataproc job
    """
    {
        "name": "TestDFSIO",
        "mapreduceJob": {
            "mainClass": "org.apache.hadoop.fs.TestDFSIO",
            "args": [
                "-write",
                "-nrFiles",
                "100",
                "-fileSize",
                "1000"
                ],
            "properties": {
                "test.build.data": "/tmp/testdfsio"
            }
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And job driver output contains
    """
    completed successfully
    """
    Then print job results

  @testdfsio_benchmark @read @hdfs
  Scenario: Benchmark read from HDFS
    Given cluster "benchmark_hdfs_io_cluster" is up and running
    When we run dataproc job
    """
    {
        "name": "TestDFSIO",
        "mapreduceJob": {
            "mainClass": "org.apache.hadoop.fs.TestDFSIO",
            "args": [
                "-read",
                "-nrFiles",
                "100",
                "-fileSize",
                "1000"
                ],
            "properties": {
                "test.build.data": "/tmp/testdfsio"
            }
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "10 minutes"
    And job driver output contains
    """
    completed successfully
    """
    Then print job results

  @testdfsio_benchmark @write @hdfs @rf3
  Scenario: Benchmark write to HDFS
    Given cluster "benchmark_hdfs_io_cluster" is up and running
    When we run dataproc job
    """
    {
        "name": "TestDFSIO",
        "mapreduceJob": {
            "mainClass": "org.apache.hadoop.fs.TestDFSIO",
            "args": [
                "-write",
                "-nrFiles",
                "100",
                "-fileSize",
                "1000"
                ],
            "properties": {
                "dfs.replication": "3",
                "test.build.data": "/tmp/testdfsio"
            }
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "110 minutes"
    And job driver output contains
    """
    completed successfully
    """
    Then print job results

  @testdfsio_benchmark @read @hdfs @rf3
  Scenario: Benchmark read from HDFS
    Given cluster "benchmark_hdfs_io_cluster" is up and running
    When we run dataproc job
    """
    {
        "name": "TestDFSIO",
        "mapreduceJob": {
            "mainClass": "org.apache.hadoop.fs.TestDFSIO",
            "args": [
                "-read",
                "-nrFiles",
                "100",
                "-fileSize",
                "1000"
                ],
            "properties": {
                "dfs.replication": "3",
                "test.build.data": "/tmp/testdfsio"
            }
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "150 minutes"
    And job driver output contains
    """
    completed successfully
    """
    Then print job results

  @delete @hdfs
  Scenario: Delete Data Proc cluster
    Given cluster "benchmark_hdfs_io_cluster" exists
    When we try to remove cluster
    Then response should have status 200
    And generated task is finished within "3 minutes"

  @create @s3
  Scenario: Dataproc cluster created successfully
    When we try to create cluster "benchmark_s3_io_cluster"
    """
    shortcuts:
      properties: {
        'core:fs.default.name': 's3a://dataproc-e2e/',
        'core:fs.defaultFS': 's3a://dataproc-e2e/'
      }
    """
    Then response should have status 200
    And generated task is finished within "10 minutes"

  @testdfsio_benchmark @write @s3
  Scenario: Benchmark write to S3
    Given cluster "benchmark_s3_io_cluster" is up and running
    When we run dataproc job
    """
    {
        "name": "TestDFSIO",
        "mapreduceJob": {
            "mainClass": "org.apache.hadoop.fs.TestDFSIO",
            "args": [
                "-write",
                "-nrFiles",
                "100",
                "-fileSize",
                "1000"
                ],
            "properties": {
                "test.build.data": "s3a://dataproc-e2e/tmp/testdfsio"
            }
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"
    And job driver output contains
    """
    completed successfully
    """
    Then print job results

  @testdfsio_benchmark @read @s3
  Scenario: Benchmark read from S3
    Given cluster "benchmark_s3_io_cluster" is up and running
    When we run dataproc job
    """
    {
        "name": "TestDFSIO",
        "mapreduceJob": {
            "mainClass": "org.apache.hadoop.fs.TestDFSIO",
            "args": [
                "-read",
                "-nrFiles",
                "100",
                "-fileSize",
                "1000"
                ],
            "properties": {
                "test.build.data": "s3a://dataproc-e2e/tmp/testdfsio"
            }
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "60 minutes"
    And job driver output contains
    """
    completed successfully
    """
    Then print job results

  @delete @s3
  Scenario: Delete Data Proc cluster
    Given cluster "benchmark_s3_io_cluster" exists
    When we try to remove cluster
    Then response should have status 200
    And generated task is finished within "3 minutes"
