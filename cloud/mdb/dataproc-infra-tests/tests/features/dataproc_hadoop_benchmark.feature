Feature: Dataproc cluster run banchmark

  Background: Wait until internal api is ready
    Given Internal API is up and running
    Given we are working with benchmark Hadoop cluster


  @create
  Scenario: Dataproc cluster created successfully
    When we try to create cluster "benchmark_sort_cluster"
    """
    shortcuts:
      services: HDFS, YARN, MAPREDUCE, HIVE
    """
    Then response should have status 200
    And generated task is finished within "10 minutes"
    Then update dataproc agent to the latest version


  @terasort_benchmark @copy_teraset_to_hdfs
  Scenario: Copy teragen data
    When we run dataproc job
    """
    {
        "name": "Copy Teraset",
        "hiveJob": {
            "queryList": {
                "queries": ["dfs -cp -f s3a://teragen/tera-in/part-m-00000 hdfs:///tmp/001;"]
            }
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"


  @terasort_benchmark @terasort_from_hdfs_to_hdfs
  Scenario: Benchmark HDFS using terasort example (sort data in HDFS and write result to HDFS)
    When we run dataproc job
    """
    {
        "name": "TeraSortFromHDFSToHDFS",
        "mapreduceJob": {
            "mainClass": "org.apache.hadoop.examples.terasort.TeraSort",
            "args": [
                "hdfs:///tmp/001",
                "hdfs:///user/root/tera-out-for-sort-from-hdfs-to-hdfs"
            ]
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "10 minutes"

    Given cluster "benchmark_sort_cluster" is up and running
    When we run dataproc job
    """
    {
        "name": "TeraValidateOnHDFS",
        "mapreduceJob": {
            "mainClass": "org.apache.hadoop.examples.terasort.TeraValidate",
            "args": [
                "hdfs:///user/root/tera-out-for-sort-from-hdfs-to-hdfs",
                "s3a://dataproc-e2e/tmp/teravalidate/teravalidate-for-sort-from-hdfs-to-hdfs"
            ]
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And s3 file s3://dataproc-e2e/tmp/teravalidate/teravalidate-for-sort-from-hdfs-to-hdfs/part-r-00000 contains
    """
    checksum	9ef425765d2a6f7
    """
    And s3 file s3://dataproc-e2e/tmp/teravalidate/teravalidate-for-sort-from-hdfs-to-hdfs/part-r-00000 not contains
    """
    error
    """
    And job driver output contains
    """
    completed successfully
    """


  @terasort_benchmark @terasort_from_hdfs_to_s3
  Scenario: Benchmark HDFS using terasort example (sort data in HDFS and write result to S3)
    When we run dataproc job
    """
    {
        "name": "TeraSortFromHDFSToHDFS",
        "mapreduceJob": {
            "mainClass": "org.apache.hadoop.examples.terasort.TeraSort",
            "args": [
                "hdfs:///tmp/001",
                "s3a://dataproc-e2e/tmp/teraout/tera-out-for-sort-from-hdfs-to-s3"
            ]
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "15 minutes"

    Given cluster "benchmark_sort_cluster" is up and running
    When we run dataproc job
    """
    {
        "name": "TeraValidateOnHDFS",
        "mapreduceJob": {
            "mainClass": "org.apache.hadoop.examples.terasort.TeraValidate",
            "args": [
                "s3a://dataproc-e2e/tmp/teraout/tera-out-for-sort-from-hdfs-to-s3",
                "s3a://dataproc-e2e/tmp/teravalidate/teravalidate-for-sort-from-hdfs-to-s3"
            ]
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And s3 file s3://dataproc-e2e/tmp/teravalidate/teravalidate-for-sort-from-hdfs-to-s3/part-r-00000 contains
    """
    checksum	9ef425765d2a6f7
    """
    And s3 file s3://dataproc-e2e/tmp/teravalidate/teravalidate-for-sort-from-hdfs-to-s3/part-r-00000 not contains
    """
    error
    """
    And job driver output contains
    """
    completed successfully
    """

  @terasort_benchmark @terasort_from_s3_to_s3
  Scenario: Benchmark S3 using terasort example (sort data in S3 and write result to S3)
    Given cluster "benchmark_sort_cluster" is up and running
    When we run dataproc job
    """
    {
        "name": "TeraSortOnS3",
        "mapreduceJob": {
            "mainClass": "org.apache.hadoop.examples.terasort.TeraSort",
            "args": [
                "s3a://teragen/tera-in/part-m-00000",
                "s3a://dataproc-e2e/tmp/teraout/tera-out-for-sort-from-s3-to-s3"
            ]
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "25 minutes"

    Given cluster "benchmark_sort_cluster" is up and running
    When we run dataproc job
    """
    {
        "name": "TeraValidateOnHDFS",
        "mapreduceJob": {
            "mainClass": "org.apache.hadoop.examples.terasort.TeraValidate",
            "args": [
                "s3a://dataproc-e2e/tmp/teraout/tera-out-for-sort-from-s3-to-s3",
                "s3a://dataproc-e2e/tmp/teravalidate/teravalidate-for-sort-from-s3-to-s3"
            ]
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And s3 file s3://dataproc-e2e/tmp/teravalidate/teravalidate-for-sort-from-s3-to-s3/part-r-00000 contains
    """
    checksum	9ef425765d2a6f7
    """
    And s3 file s3://dataproc-e2e/tmp/teravalidate/teravalidate-for-sort-from-s3-to-s3/part-r-00000 not contains
    """
    error
    """
    And job driver output contains
    """
    completed successfully
    """

  @terasort_benchmark @terasort_from_s3_to_hdfs
  Scenario: Benchmark S3 using terasort example (sort data in S3 and write result to HDFS)
    Given cluster "benchmark_sort_cluster" is up and running
    When we run dataproc job
    """
    {
        "name": "TeraSortOnS3",
        "mapreduceJob": {
            "mainClass": "org.apache.hadoop.examples.terasort.TeraSort",
            "args": [
                "s3a://teragen/tera-in/part-m-00000",
                "hdfs:///user/root/tera-out-for-sort-from-s3-to-hdfs"
            ]
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "20 minutes"

    Given cluster "benchmark_sort_cluster" is up and running
    When we run dataproc job
    """
    {
        "name": "TeraValidateOnHDFS",
        "mapreduceJob": {
            "mainClass": "org.apache.hadoop.examples.terasort.TeraValidate",
            "args": [
                "hdfs:///user/root/tera-out-for-sort-from-s3-to-hdfs",
                "s3a://dataproc-e2e/tmp/teravalidate/teravalidate-for-sort-from-s3-to-hdfs"
            ]
        }
    }
    """
    Then response should have status 200
    And generated task is finished within "3 minutes"
    And s3 file s3://dataproc-e2e/tmp/teravalidate/teravalidate-for-sort-from-s3-to-hdfs/part-r-00000 contains
    """
    checksum	9ef425765d2a6f7
    """
    And s3 file s3://dataproc-e2e/tmp/teravalidate/teravalidate-for-sort-from-s3-to-hdfs/part-r-00000 not contains
    """
    error
    """
    And job driver output contains
    """
    completed successfully
    """


  @delete
  Scenario: Dataproc cluster deleted successfully
    Given cluster "benchmark_sort_cluster" exists

    When we try to remove cluster
    Then response should have status 200
    And generated task is finished within "3 minutes"
