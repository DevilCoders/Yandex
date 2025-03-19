@spark @clickhouse @livy
Feature: Spark + ClickHouse integration works
    Scenario: spark cluster creates
        Given clickhouse cluster
        Given cluster name "click"
        And cluster with services: hdfs, mapreduce, yarn, spark, livy
        When cluster created within 10 minutes
        Then clickhouse created
        When tunnel to 8998 is open

    Scenario: pyspark works when reading from clickhouse
        When start livy pyspark session
        Then livy session started
        Given livy code
        """
        df = spark.read \
            .format("jdbc") \
            .option("url", "jdbc:clickhouse://{{ clickhouse_hosts[0] }}:8443/{{ clickhouse_db }}?ssl=true") \
            .option("user", "{{ clickhouse_user }}") \
            .option("password", "{{ clickhouse_password }}") \
            .option("query", "select 1") \
            .load()
        """
        When execute livy statement
        Then livy statement finished

        Given livy code
        """
        df.printSchema()
        """
        When execute livy statement
        Then livy statement finished
        And  livy output contains
        """
        root
         |-- 1: integer (nullable = true)
        """

        Given livy code
        """
        df.show()
        """
        When execute livy statement
        Then livy statement finished
        And  livy output contains
        """
        +---+
        |  1|
        +---+
        |  1|
        +---+
        """
