@hive @metastore
Feature: Environment with dedicated metastore works
    Scenario: Lightweight Hive Metastore works
        Given metastore cluster
        When cluster created within 5 minutes
        Given masternode as a metastore
        Then service hive-metastore on masternodes is running

        # Create external table on object storage
        Then execute hive query
        """
        create external table foo(id int, name string) stored as parquet location "s3a://{{ bucket }}/warehouse/foo";
        """
        # Insert some data into external table
        Then execute hive query
        """
        insert into foo values (1, "alice"), (2, "bob"), (3, "charlie");
        """
        
    Scenario: Lightweight Spark with external hive metastore works
        Given lightweight spark cluster
        And cluster name "spark"
        And property spark:spark.hive.metastore.uris = thrift://{{ metastore }}.ru-central1.internal:9083

        When cluster created within 10 minutes
        And tunnel to 8998 is open
    
        When start livy spark session session-1
        Then livy session started
        Given livy code
        """
        spark.sql("show tables").show()
        """
        When execute livy statement
        Then livy statement finished
        And  livy output contains
        """
        +--------+---------+-----------+
        |database|tableName|isTemporary|
        +--------+---------+-----------+
        | default|      foo|      false|
        +--------+---------+-----------+
        """
        Given livy code
        """
        spark.sql("select * from foo").show()
        """
        When execute livy statement
        Then livy statement finished
        And  livy output contains
        """
        +---+-------+
        | id|   name|
        +---+-------+
        |  1|  alice|
        |  2|    bob|
        |  3|charlie|
        +---+-------+
        """
        Then stop livy session
