@ppa @trunk
Feature: Cluster with customized repositories and packages works
    Scenario: Cluster with ppa and apt works
        Given cluster name "ppa"
        Given NAT network
        And cluster with services: hdfs, mapreduce, yarn, spark
        And property dataproc:ppa = deadsnakes/ppa
        And property dataproc:apt = python3.6,python3.6-venv,python3.6-examples
        When cluster created

    Scenario: custom packages installed
        Then package python3.6 installed
        Then package python3.6-venv installed
        Then package python3.6-examples installed

    Scenario: spark pi on python3.6 works
        Given file /home/ubuntu/spark-pi.py
        """
        import sys
        import random
        import pyspark
        from pyspark.sql import SparkSession

        SAMPLES = 10000

        def inside(p):
            x, y = random.random(), random.random()
            return x*x + y*y < 1

        sc = SparkSession.builder.getOrCreate().sparkContext

        count  = sc.parallelize(range(0, SAMPLES)).filter(inside).count()
        print("Pi is roughly %f" % (4.0 * count / SAMPLES))
        print(f"Python version is {sys.version}")
        """
        When execute command
        """
        PYSPARK_PYTHON=python3.6 HADOOP_USER_NAME=spark spark-submit spark-pi.py --deploy-mode client
        """
        Then command finishes within 120 seconds
        And command stdout contains
        """
        Pi is roughly 3.
        """
        And command stdout contains
        """
        Python version is 3.6
        """