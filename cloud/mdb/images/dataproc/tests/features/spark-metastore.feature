@metastore @spark
Feature: Metastore works with spark
    Scenario: Cluster with hive and spark creates
        Given cluster name "metastore"
        # NAT needs for working with object storage
        And NAT network
        And cluster topology is lightweight
        And cluster with services: yarn, hive, spark, livy
        When cluster created within 10 minutes
        Then service livy on masternodes is running
        When tunnel to 8998 is open

    Scenario: database testdb1 created
        Then execute hive query
        """
        drop database if exists testdb1;
        """
        And execute hive query
        """
        create database testdb1 location "s3a://{{ bucket }}/testdb1";
        """

    Scenario: testdb1 visible from other spark sessions
        When start livy spark session session-2
        Then livy session started
        Given livy code
        """
        spark.sql("show databases").show()
        """
        When execute livy statement
        Then livy statement finished
        And  livy output contains
        """
        +---------+
        |namespace|
        +---------+
        |  default|
        |  testdb1|
        +---------+
        """
        Then stop livy session

    Scenario: create table from s3
        Given s3 object cars.csv from datasets/cars.csv
        When start livy pyspark session session-3
        Then livy session started
        Given livy code
        """
        df = spark.read.option('header', 'true').option('inferSchema', 'true').csv('s3a://{{ bucket }}/cars.csv')
        df.show()
        """
        When execute livy statement
        Then livy statement finished
        And  livy output contains
        """"
        +--------------------+----+---------+------------+----------+------+------------+-----+------+
        |                 Car| MPG|Cylinders|Displacement|Horsepower|Weight|Acceleration|Model|Origin|
        +--------------------+----+---------+------------+----------+------+------------+-----+------+
        |Chevrolet Chevell...|18.0|        8|       307.0|     130.0|  3504|        12.0|   70|    US|
        |   Buick Skylark 320|15.0|        8|       350.0|     165.0|  3693|        11.5|   70|    US|
        |  Plymouth Satellite|18.0|        8|       318.0|     150.0|  3436|        11.0|   70|    US|
        |       AMC Rebel SST|16.0|        8|       304.0|     150.0|  3433|        12.0|   70|    US|
        |         Ford Torino|17.0|        8|       302.0|     140.0|  3449|        10.5|   70|    US|
        |    Ford Galaxie 500|15.0|        8|       429.0|     198.0|  4341|        10.0|   70|    US|
        |    Chevrolet Impala|14.0|        8|       454.0|     220.0|  4354|         9.0|   70|    US|
        |   Plymouth Fury iii|14.0|        8|       440.0|     215.0|  4312|         8.5|   70|    US|
        |    Pontiac Catalina|14.0|        8|       455.0|     225.0|  4425|        10.0|   70|    US|
        |  AMC Ambassador DPL|15.0|        8|       390.0|     190.0|  3850|         8.5|   70|    US|
        |Citroen DS-21 Pallas| 0.0|        4|       133.0|     115.0|  3090|        17.5|   70|Europe|
        |Chevrolet Chevell...| 0.0|        8|       350.0|     165.0|  4142|        11.5|   70|    US|
        |    Ford Torino (sw)| 0.0|        8|       351.0|     153.0|  4034|        11.0|   70|    US|
        |Plymouth Satellit...| 0.0|        8|       383.0|     175.0|  4166|        10.5|   70|    US|
        |  AMC Rebel SST (sw)| 0.0|        8|       360.0|     175.0|  3850|        11.0|   70|    US|
        | Dodge Challenger SE|15.0|        8|       383.0|     170.0|  3563|        10.0|   70|    US|
        |  Plymouth 'Cuda 340|14.0|        8|       340.0|     160.0|  3609|         8.0|   70|    US|
        |Ford Mustang Boss...| 0.0|        8|       302.0|     140.0|  3353|         8.0|   70|    US|
        |Chevrolet Monte C...|15.0|        8|       400.0|     150.0|  3761|         9.5|   70|    US|
        |Buick Estate Wago...|14.0|        8|       455.0|     225.0|  3086|        10.0|   70|    US|
        +--------------------+----+---------+------------+----------+------+------------+-----+------+
        only showing top 20 rows
        """
        Given livy code
        """
        df.printSchema()
        """
        When execute livy statement
        Then livy statement finished
        And  livy output contains
        """
        root
         |-- Car: string (nullable = true)
         |-- MPG: double (nullable = true)
         |-- Cylinders: integer (nullable = true)
         |-- Displacement: double (nullable = true)
         |-- Horsepower: double (nullable = true)
         |-- Weight: decimal(4,0) (nullable = true)
         |-- Acceleration: double (nullable = true)
         |-- Model: integer (nullable = true)
         |-- Origin: string (nullable = true)
        """
        Given livy code
        """
        df.write.mode('overwrite').saveAsTable('cars')
        """
        When execute livy statement
        Then livy statement finished
        Then stop livy session
    
    Scenario: try to read from
        When start livy spark session session-4
        Then livy session started
        Given livy code
        """
        spark.sql("select Origin, int(avg(mpg)) as MPG, int(avg(Horsepower)) as Horsepower, count(*) as Count from cars group by Origin order by Count desc").show();
        """
        Then livy statement finished
        And livy output contains
        """
        +------+---+----------+-----+
        |Origin|MPG|Horsepower|Count|
        +------+---+----------+-----+
        |    US| 19|       118|  254|
        | Japan| 30|        79|   79|
        |Europe| 26|        78|   73|
        +------+---+----------+-----+
        """
